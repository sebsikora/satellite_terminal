// -----------------------------------------------------------------------------------------------------
// satellite_terminal - Easily spawn and communicate bidirectionally with client processes in separate
//                      terminal emulator instances.
// -----------------------------------------------------------------------------------------------------
// seb.nf.sikora@protonmail.com
//
// Copyright © 2021 Dr Seb N.F. Sikora.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.
// -----------------------------------------------------------------------------------------------------

#include <iostream>
#include <fstream>                    // std::ifstream.
#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include <errno.h>                    // errno.
#include <unistd.h>                   // fork(), execl(), getcwd().

#include "satellite_terminal.h"

SatTerm_Server::SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages, 
                               std::string const& path_to_terminal_emulator_paths, std::vector<std::string> out_port_identifiers,
                               std::vector<std::string> in_port_identifiers, char end_char, std::string const& stop_port_identifier,
                               std::string const& stop_message) {
	
	m_identifier = identifier;
	m_display_messages = display_messages;
	m_end_char = end_char;
	m_stop_message = stop_message;
	
	if (out_port_identifiers.size() == 0) {
		out_port_identifiers.push_back("server_tx");
	}
	if (in_port_identifiers.size() == 0) {
		in_port_identifiers.push_back("server_rx");
	}
	m_default_port.tx = out_port_identifiers[0];
	m_default_port.rx = in_port_identifiers[0];
	
	if (stop_port_identifier == "") {
		m_stop_port_identifier = m_default_port.tx;
	}
	
	m_working_path = GetWorkingPath();
	
	if (m_working_path != "") {
		
		bool success = CreatePorts(true, true, m_working_path, in_port_identifiers, m_display_messages, m_end_char, m_in_ports);
		if (success) {
			success = CreatePorts(true, false, m_working_path, out_port_identifiers, m_display_messages, m_end_char, m_out_ports);
		}
		
		if (success) {
			pid_t client_pid = StartClient(path_to_terminal_emulator_paths, path_to_client_binary, m_working_path, m_end_char, m_stop_message,
			                               in_port_identifiers, out_port_identifiers);
			if (client_pid < 0) {
				success = false;
			}
		}
		
		unsigned long timeout_seconds = 5;
		if (success) {
			success = OpenPorts(m_in_ports, timeout_seconds);
		}
		
		if (success) {
			success = OpenPorts(m_out_ports, timeout_seconds);
		}
		
		if (success) {
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " successfully initialised connection.";
				std::cerr << message << std::endl;
			}
		} else {
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " unable to intialise connection.";
				std::cerr << message << std::endl;
			}
		}
		SetConnectedFlag(success);
	}
}

SatTerm_Server::~SatTerm_Server() {
	if (IsConnected()) {
		SendMessage(m_stop_message, m_stop_port_identifier);
		if (m_display_messages) {
			std::cerr << "Waiting for client process to terminate..." << std::endl;
		}
		// Poll for EOF on read on m_default_port.rx (tells us that client write end has closed).
		while(IsConnected()) {
			GetMessage(false, 0);
		}
	}
	// There should not be any more to do. Pointers in m_in_ports and m_out_ports are stored as unique_ptr<Port>, so when the maps
	// are destroyed now the destructors for the Ports should be called automatically. In these the fifos will be unlink()ed if m_is_server is set.
}

std::string SatTerm_Server::GetWorkingPath(void) {
	m_error_code = {0, ""};
	
	char working_path[FILENAME_MAX + 1];
	char* retval = getcwd(working_path, FILENAME_MAX + 1);
	if (retval == NULL) {
		m_error_code = {errno, "getcwd()"};
		if (m_display_messages) {
			perror("getcwd() unable to obtain current working path.");
		}
		return "";
	} else {
		std::string working_path_string = std::string(working_path);
		if (working_path_string.back() != '/') {
			working_path_string += "/";
		}
		if (m_display_messages) {
			std::cerr << "Server working path is " << working_path_string << std::endl;
		}
		return working_path_string;
	}
}

pid_t SatTerm_Server::StartClient(std::string const& path_to_terminal_emulator_paths, std::string const& path_to_client_binary, std::string const& working_path,
                                  char end_char, std::string const& stop_message, std::vector<std::string> in_port_identifiers, std::vector<std::string> out_port_identifiers) {
	m_error_code = {0, ""};
	
	std::vector<std::string> terminal_emulator_paths = LoadTerminalEmulatorPaths(path_to_terminal_emulator_paths);
	
	pid_t process;
	if (terminal_emulator_paths.size() > 0) {
		process = fork();
		if (process < 0) {        // fork() failed.
			// Info on possible fork() errors - https://pubs.opengroup.org/onlinepubs/009696799/functions/fork.html
			m_error_code = {errno, "fork()"};
			if (m_display_messages) {
				perror("fork() to client process failed.");
			}
			return process;
		}
		if (process == 0) {       // We are in the child process!
			if (m_display_messages) {
				std::string message = "Client process started.";
				std::cerr << message << std::endl;
			}
			
			// Assemble command string to be passed to terminal emulator.
			std::string arg_string = path_to_client_binary;
			arg_string += " client_args";    // Argument start delimiter.
			arg_string += " " + working_path;
			arg_string += " " + std::to_string((int)(end_char));
			arg_string += " " + stop_message;
			arg_string += " " + std::to_string(in_port_identifiers.size());
			arg_string += " " + std::to_string(out_port_identifiers.size());
			
			for (const auto& identifier : in_port_identifiers) {
				arg_string += " " + identifier;
			}
			for (const auto& identifier : out_port_identifiers) {
				arg_string += " " + identifier;
			}
			
			if (m_display_messages) {
				std::cerr << "Client process attempting to execute via terminal emulator '-e':" << std::endl << arg_string << std::endl;
			}
			
			for (const auto terminal_path : terminal_emulator_paths) {
				// No need to check execl() return value. If it returns, you know it failed.
				if (m_display_messages) {
					std::cerr << "Trying " << terminal_path << std::endl;
				}
				execl(terminal_path.c_str(), terminal_path.c_str(), "-e", arg_string.c_str(), (char*) NULL);
			}
			
			if (m_display_messages) {
				// No point storing m_error_code, we are in the child process...
				std::string error_string = "Client process execl() failed to start client binary. Check terminal_emulator_paths.txt";
				perror(error_string.c_str());
			}
			std::exit(1);    // Have to exit(1) here to terminate client process if we couldn't start a terminal emulator.
			return -1;
		}
	} else {
		return -1;
	}
    return process;
}

std::vector<std::string> SatTerm_Server::LoadTerminalEmulatorPaths(std::string const& file_path) {
	m_error_code = {0, ""};
	
	std::vector<std::string> terminal_emulator_paths = {};
	std::ifstream data_file (file_path);
	std::string file_row = "";

	if (data_file.is_open()) {
		while (std::getline(data_file, file_row)) {
			terminal_emulator_paths.push_back(file_row);
		}
		data_file.close();
	} else {
		m_error_code = {-1, "no_terminal_emulator_paths_file" + file_path};
		if (m_display_messages) {
			std::cerr << "Server unable to open terminal emulator paths file at " << file_path << std::endl;
		}
	}
	return terminal_emulator_paths;
}
