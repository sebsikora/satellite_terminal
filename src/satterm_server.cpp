/*
	
	satellite_terminal - Easily spawn and communicate bidirectionally with client processes in separate terminal emulator instances.
	
	Copyright © 2021 Dr Seb N.F. Sikora.
	
	satellite_terminal is distributed under the terms of the MIT license.
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
	is furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
	IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
	seb.nf.sikora@protonmail.com
	
*/

#include <stdio.h>                    // perror().
#include <fcntl.h>                    // open() and O_RDONLY, O_WRONLY, etc.
#include <unistd.h>                   // write(), read(), sleep(), fork(), execl(), close(), unlink().
#include <sys/stat.h>                 // mkfifo().
#include <stdlib.h>                   // exit().
#include <errno.h>                    // errno.
#include <signal.h>                   // SIGPIPE, SIG_IGN.

#include <string>                     // std::string.
#include <vector>                     // std::vector.
#include <iostream>                   // std::cout, std::cerr, std::endl.
#include <fstream>                    // std::ifstream.
#include <cstdio>                     // remove().

#include "satellite_terminal.h"

// Class constructor and member function definitions for derived Server class.

SatTerm_Server::SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages, size_t stop_fifo_index, size_t tx_fifo_count, size_t rx_fifo_count, char end_char, std::string const& stop_message) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	
	signal(SIGPIPE, SIG_IGN);    // If the reader at the other end of the pipe closes prematurely, when we try and write() to the pipe
		                         // a SIGPIPE signal is generated and this process terminates.
                                 // We call signal() here to prevent the signal from being raised as-per https://stackoverflow.com/a/9036323
                                 // After each write() call we need to check the return value and if -1 check for the EPIPE error code
                                 // before/if writing again.
	m_component_type = "Server";
	m_path_to_client_binary = path_to_client_binary;
	m_end_char = end_char;
	m_stop_fifo_index = stop_fifo_index;
	m_stop_message = stop_message;
	
	bool success = CreateFifos(tx_fifo_count, rx_fifo_count);

	if (success) {
		if (StartClient("./terminal_emulator_paths.txt") < 0) {
			m_error_code = {1, "fork()"};
			if (m_display_messages) {
				std::string message = "Unable to start client process.";
				std::cout << message << std::endl;
			}
			success = false;
		} else {
			if (m_display_messages) {
				std::string message = "Client process started.";
				std::cout << message << std::endl;
			}
		}
	} else {
		if (m_display_messages) {
			std::string message = "Unable to create fifo(s).";
			std::cout << message << std::endl;
		}
	}
	if (success) {
		unsigned long timeout_seconds = 5;
		success = OpenFifos(timeout_seconds);
		if (success) {
			m_connected = true;
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " initialised successfully.";
				std::cout << message << std::endl;
			}
		} else {
			m_connected = false;
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " unable to intialise connection.";
				std::cout << message << std::endl;
			}
		}
	} else {
		m_connected = false;
	}
}

SatTerm_Server::~SatTerm_Server() {
	if (IsConnected()) {
		SendMessage(m_stop_message, m_stop_fifo_index);
		if (m_display_messages) {
			std::cout << "Waiting for client process to terminate..." << std::endl;
		}
	}
	// Poll for EOF on read on fifo 0 (tells us that client write end has closed).
	while(IsConnected()) {
		GetMessage(0, false, 0);
	}
	// Close and delete FIFO buffers.
	for (size_t fifo_index = 0; fifo_index < m_rx_fifo_descriptors.size(); fifo_index ++) {
		close(m_rx_fifo_descriptors[fifo_index]);
		int e = unlink(m_rx_fifo_paths[fifo_index].c_str());
		if ((e < 0) && m_display_messages) {
			std::string error_message = "Unable to unlink fifo at path " + m_rx_fifo_paths[fifo_index];
			perror(error_message.c_str());
		}
		remove(m_rx_fifo_paths[fifo_index].c_str());		// If temporary file still exists, delete it.
	}
	
	for (size_t fifo_index = 0; fifo_index < m_tx_fifo_descriptors.size(); fifo_index ++) {
		close(m_tx_fifo_descriptors[fifo_index]);
		int e = unlink(m_tx_fifo_paths[fifo_index].c_str());
		if ((e < 0) && m_display_messages) {
			std::string error_message = "Unable to unlink fifo at path " + m_tx_fifo_paths[fifo_index];
			perror(error_message.c_str());
		}
		remove(m_tx_fifo_paths[fifo_index].c_str());		// If temporary file still exists, delete it.
	}
}

bool SatTerm_Server::CreateFifos(size_t tx_fifo_count, size_t rx_fifo_count) {
	m_error_code = {0, ""};
	bool success = true;
	int status = 0;
	
	for (size_t tx_fifo_index = 0; tx_fifo_index < tx_fifo_count; tx_fifo_index ++) {
		std::string fifo_path = m_identifier + "_fifo_sc_" + std::to_string(tx_fifo_index);
		remove(fifo_path.c_str());	// If temporary file already exists, delete it.
		m_tx_fifo_paths.emplace_back(fifo_path);
		
		status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);
		
		if (status < 0) {
			m_error_code = {errno, "mkfifo()"};
			if (m_display_messages) {
				std::string error_message = "Unable to open fifo at path " + fifo_path;
				perror(error_message.c_str());
			}
			success = false;
			break;
		}
	}
	
	if (success) {
		for (size_t rx_fifo_index = 0; rx_fifo_index < rx_fifo_count; rx_fifo_index ++) {
			std::string fifo_path = m_identifier + "_fifo_cs_" + std::to_string(rx_fifo_index);
			remove(fifo_path.c_str());	// If temporary file already exists, delete it.
			m_rx_fifo_paths.emplace_back(fifo_path);
			
			status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);
			
			if (status < 0) {
				m_error_code = {errno, "mkfifo()"};
				if (m_display_messages) {
					std::string error_message = "Unable to open fifo at path " + fifo_path;
					perror(error_message.c_str());
				}
				success = false;
				break;
			}
		}
	}
	return success;
}

pid_t SatTerm_Server::StartClient(std::string const& path_to_terminal_emulator_paths) {
	m_error_code = {0, ""};
	bool success = true;
	
	char working_path[FILENAME_MAX + 1];
	char* retval = getcwd(working_path, FILENAME_MAX + 1);
	if (retval == NULL) {
		m_error_code = {errno, "getcwd()"};
		if (m_display_messages) {
			perror("getcwd() unable to obtain current working path.");
		}
		success = false;
	} else {
		if (m_display_messages) {
			std::cout << "Fifo working path is " << std::string(working_path) << std::endl;
		}
	}
	
	std::vector<std::string> terminal_emulator_paths = LoadTerminalEmulatorPaths(path_to_terminal_emulator_paths);
	
	pid_t process;
	if (success) {
		if (terminal_emulator_paths.size() > 0) {
			process = fork();
			if (process < 0) {
				m_error_code = {errno, "fork()"};
				if (m_display_messages) {
					perror("fork() to start client process failed."); // fork() failed.
				}
				return process;
			}
			
			if (process == 0) {
				// We are in the child process!
				std::string arg_string = m_path_to_client_binary;
				arg_string += " client_args";
				arg_string += " " + std::string(working_path) + "/";
				arg_string += " " + m_stop_message;
				arg_string += " " + std::to_string(m_rx_fifo_paths.size());
				arg_string += " " + std::to_string(m_tx_fifo_paths.size());
				for (const auto& fifo_path : m_rx_fifo_paths) {
					arg_string += " " + fifo_path;
				}
				for (const auto& fifo_path : m_tx_fifo_paths) {
					arg_string += " " + fifo_path;
				}
				
				//~std::cout << "Full command is " << arg_string << std::endl;
				for (const auto terminal_path : terminal_emulator_paths) {
					// No need to check execv() return value. If it returns, you know it failed.
					execl(terminal_path.c_str(), terminal_path.c_str(), "-e", arg_string.c_str(), (char*) NULL);
				}
				if (m_display_messages) {
					std::string error_string = "Client process execl() failed to start client binary. Check terminal_emulator_paths.txt";
					perror(error_string.c_str());
				}
				std::exit(1);		// Have to exit(1) here to terminate client process if we couldn't start a terminal emulator.
				return -1;
			}
		} else {
			return -1;
		}
	} else {
		return -1;
	}
    return process;
}

bool SatTerm_Server::OpenFifos(unsigned long timeout_seconds) {
	bool success = false;
	success = OpenRxFifos(timeout_seconds);

	if (success) {		// Only attempt to open fifos for writing if we were able to open the fifos for reading.
		success = OpenTxFifos(timeout_seconds);
	}
	return success;
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
		m_error_code = {-1, "Unable to open terminal emulator paths file at " + file_path};
		if (m_display_messages) {
			std::cout << "Unable to open terminal emulator paths file at " << file_path << std::endl;
		}
	}
	return terminal_emulator_paths;
}
