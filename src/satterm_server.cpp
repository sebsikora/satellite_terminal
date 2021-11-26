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

#include <stdio.h>					// perror().
#include <fcntl.h>					// open() and O_RDONLY, O_WRONLY, etc.
#include <unistd.h>					// write(), read(), sleep(), fork(), execl(), close(), unlink().
#include <sys/stat.h>				// mkinfo().
#include <stdlib.h>					// mkinfo(), exit().
#include <sys/wait.h>				// wait().
#include <errno.h>					// errno.

#include <string>					// std::string.
#include <vector>					// std::vector.
#include <iostream>					// std::cout, std::cerr, std::endl.
#include <cstdio>					// remove().

#include "satellite_terminal.h"

SatTerm_Server::SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, char end_char, std::string const& stop_signal, bool display_messages, size_t stop_fifo_index, size_t tx_fifo_count, size_t rx_fifo_count)
 : SatTerm_Component(identifier, display_messages) {
	m_component_type = "Server";
	m_path_to_client_binary = path_to_client_binary;
	m_end_char = end_char;
	m_stop_fifo_index = stop_fifo_index;
	m_stop_signal = stop_signal;
	
	bool success = CreateFifos(tx_fifo_count, rx_fifo_count);

	if (success) {
		if ((m_client_pid = StartClient()) < 0) {
			std::string message = "Unable to start client process.";
			std::cout << message << std::endl;
			if (m_display_messages) {
				success = false;
			}
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
		success = OpenFifos();
		if (success) {
			m_initialised_successfully = true;
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " initialised successfully.";
				std::cout << message << std::endl;
			}
			//~for (int tx_fifo_index = 0; tx_fifo_index < tx_fifo_count; tx_fifo_index ++) {
				//~std::string message("Satellite terminal server " + identifier + " checking in.");
				//~SendMessage(message, tx_fifo_index);
			//~}
		} else {
			m_initialised_successfully = false;
			if (m_display_messages) {
				std::string message = "Server " + m_identifier + " unable to open fifo(s).";
				std::cout << message << std::endl;
			}
		}
	} else {
		m_initialised_successfully = false;
	}
}

SatTerm_Server::~SatTerm_Server() {
	if (m_initialised_successfully) {
		Stop();
	}
}

bool SatTerm_Server::CreateFifos(size_t tx_fifo_count, size_t rx_fifo_count) {
	m_error_code = 0;
	bool success = true;
	int status = 0;
	for (size_t tx_fifo_index = 0; tx_fifo_index < tx_fifo_count; tx_fifo_index ++) {
		std::string fifo_path = "./" + m_identifier + "_fifo_sc_" + std::to_string(tx_fifo_index);
		remove(fifo_path.c_str());	// If temporary file already exists, delete it.
		m_tx_fifo_paths.emplace_back(fifo_path);
		status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);
		if (status < 0) {
			m_error_code = errno;
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
			std::string fifo_path = "./" + m_identifier + "_fifo_cs_" + std::to_string(rx_fifo_index);
			remove(fifo_path.c_str());	// If temporary file already exists, delete it.
			m_rx_fifo_paths.emplace_back(fifo_path);
			status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);
			if (status < 0) {
				m_error_code = errno;
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

pid_t SatTerm_Server::StartClient() {
	pid_t process = fork();
    if (process < 0)
    {
		m_error_code = errno;
        if (m_display_messages) {
			perror("fork() failed to start client process failed."); // fork() failed.
		}
        return process;
    }
    if (process == 0) {
		// We are in the child process!
        std::string arg_string = m_path_to_client_binary;
        arg_string += " " + std::to_string(m_rx_fifo_paths.size());
        arg_string += " " + std::to_string(m_tx_fifo_paths.size());
        for (const auto& fifo_path : m_rx_fifo_paths) {
			arg_string += " " + fifo_path;
		}
		for (const auto& fifo_path : m_tx_fifo_paths) {
			arg_string += " " + fifo_path;
		}
		// No need to check execv() return value. If it returns, you know it failed.
		execl("/usr/bin/x-terminal-emulator", "/usr/bin/x-terminal-emulator", "-e", arg_string.c_str(), (char*) NULL);
		execl("/usr/bin/terminator", "/usr/bin/terminator", "-e", arg_string.c_str(), (char*) NULL);	// Fail through to terminator if x-terminal-emulator not available.
        execl("/usr/bin/xterm", "/usr/bin/xterm", "-e", arg_string.c_str(), (char*) NULL);	// Fail through to xterm if terminator not available.
        perror("Client process execl() failed to start client binary.");
        std::exit(1);		// Have to exit() here to terminate client process if we couldn't start a terminal emulator.
        return -1;
    }
    return process;
}

bool SatTerm_Server::OpenFifos() {
	bool success = false;
	success = OpenRxFifos();
	if (success) {		// Only attempt to open fifos for writing if we were able to open the fifos for reading.
		success = OpenTxFifos();
	}
	return success;
}

int SatTerm_Server::Stop() {
	SendMessage(m_stop_signal, m_stop_fifo_index);
	int status = Finish();
	return status;
}

int SatTerm_Server::Finish() {
	int status;
	pid_t wait_result = waitpid(m_client_pid, &status, 0);  // Parent process waits here for child to terminate.
	if (m_display_messages) {
		std::cout << "Client process finished." << std::endl;
		std::cout << "Process " << (unsigned long) wait_result << " returned result: " << status << std::endl;
	}
	// Close and delete FIFO buffers.
	for (size_t fifo_index = 0; fifo_index < m_rx_fifo_descriptors.size(); fifo_index ++) {
		close(m_rx_fifo_descriptors[fifo_index]);
		int e = unlink(m_rx_fifo_paths[fifo_index].c_str());
		if (e < 0) {
			std::string error_message = "Unable to unlink fifo at path " + m_rx_fifo_paths[fifo_index];
			perror(error_message.c_str());
		}
		remove(m_rx_fifo_paths[fifo_index].c_str());		// If temporary file still exists, delete it.
	}
	for (size_t fifo_index = 0; fifo_index < m_tx_fifo_descriptors.size(); fifo_index ++) {
		close(m_tx_fifo_descriptors[fifo_index]);
		int e = unlink(m_tx_fifo_paths[fifo_index].c_str());
		if (e < 0) {
			std::string error_message = "Unable to unlink fifo at path " + m_tx_fifo_paths[fifo_index];
			perror(error_message.c_str());
		}
		remove(m_tx_fifo_paths[fifo_index].c_str());		// If temporary file still exists, delete it.
	}
	
	return status;
}
