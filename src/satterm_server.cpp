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

#include <iostream>                   // std::cout, std::cerr, std::endl.
#include <fstream>                    // std::ifstream.
#include <string>                     // std::string, std::to_string.
#include <map>                        // std::map.
#include <vector>                     // std::vector.
#include <memory>                     // std::unique_ptr.

#include <errno.h>                    // errno.
#include <unistd.h>                   // fork(), execl(), getcwd().
#include <stdio.h>                    // perror(), FILENAME_MAX.

#include "satellite_terminal.h"

SatTerm_Server::SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages,
                               std::vector<std::string> port_identifiers, std::string const& stop_message,
                               std::string const& path_to_terminal_emulator_paths, char end_char, std::string const& stop_port_identifier,
                               unsigned long timeout_seconds) {
	
	m_identifier = identifier;
	m_display_messages = display_messages;
	m_end_char = end_char;
	m_stop_message = stop_message;
	
	if (port_identifiers.size() == 0) {
		port_identifiers.push_back("comms");
	}
	m_default_port_identifier = port_identifiers[0];
	
	if (stop_port_identifier == "") {
		m_stop_port_identifier = m_default_port_identifier;
	}
	
	m_working_path = GetWorkingPath();
	
	bool success = true;
	if (m_working_path != "") {
		
		if (success) {
			pid_t client_pid = StartClient(path_to_terminal_emulator_paths, path_to_client_binary, m_working_path, m_end_char, m_stop_message, port_identifiers);
			if (client_pid < 0) {
				success = false;
			}
		}
		
		if (success) {
			success = CreatePorts(true, m_working_path, port_identifiers, m_display_messages, m_end_char, m_ports);
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
	} else {
		success = false;
	}
	SetConnectedFlag(success);
}

SatTerm_Server::~SatTerm_Server() {
	if (IsConnected()) {
		SendMessage(m_stop_message, m_stop_port_identifier);
		if (m_display_messages) {
			std::cerr << "Waiting for client process to terminate..." << std::endl;
		}
		
		unsigned long start_time = time(0);
		while(IsConnected() && ((time(0) - start_time) < 5)) {
			std::string shutdown_confirmation = GetMessage(m_stop_port_identifier, false, 0);
			if (shutdown_confirmation == m_stop_message) {
				break;
			}
		}
	}
}

std::string SatTerm_Server::GetWorkingPath(void) {
	m_error_code = {0, ""};
	
	char working_path[FILENAME_MAX + 1];
	char* retval = getcwd(working_path, FILENAME_MAX + 1);
	if (retval == NULL) {
		m_error_code = {errno, "getcwd()"};
		if (m_display_messages) {
			perror("getcwd() unable to obtain current working path");
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
                                  char end_char, std::string const& stop_message, std::vector<std::string> port_identifiers) {
	m_error_code = {0, ""};
	
	std::vector<std::string> terminal_emulator_paths = LoadTerminalEmulatorPaths(path_to_terminal_emulator_paths);
	
	pid_t process;
	if (terminal_emulator_paths.size() > 0) {
		process = fork();
		if (process < 0) {        // fork() failed.
			// Info on possible fork() errors - https://pubs.opengroup.org/onlinepubs/009696799/functions/fork.html
			m_error_code = {errno, "fork()"};
			if (m_display_messages) {
				perror("fork() to client process failed");
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
			arg_string += " " + std::to_string(port_identifiers.size());
			
			for (const auto& identifier : port_identifiers) {
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
