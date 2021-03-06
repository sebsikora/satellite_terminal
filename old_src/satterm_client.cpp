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

#include <string>              // std::string.
#include <vector>              // std::vector.
#include <iostream>            // std::cout, std::cerr, std::endl.
#include <stdexcept>           // std::out_of_range, std::invalid_argument.

#include <signal.h>            // SIGPIPE, SIG_IGN.
#include <unistd.h>            // write(), read(), close().

#include "satellite_terminal.h"

// Class constructor and member function definitions for derived Client class.

// Construct a client by parsing argv from the stipulated argument index (inclusive).
SatTerm_Client::SatTerm_Client(std::string const& identifier, int argc, char* argv[], bool display_messages) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	
	signal(SIGPIPE, SIG_IGN);    // If the reader at the other end of the pipe closes prematurely, when we try and write() to the pipe
		                         // a SIGPIPE signal is generated and this process terminates.
                                 // We call signal() here to prevent the signal from being raised as-per https://stackoverflow.com/a/9036323
                                 // After each write() call we need to check the return value and if -1 check for the EPIPE error code
                                 // before/if writing again.
	m_component_type = "Client";
	
	size_t argv_start_index = GetArgStartIndex(argc, argv);
	
	if (argv_start_index != 0) {
		// Parse argv - Parameters are sanity checked by server but we'll try and catch appropriate exceptions in case the args became garbled in transmission.
		bool success = true;
		try {
			m_working_path = std::string(argv[argv_start_index]);
			m_stop_fifo_index = std::stoi(std::string(argv[argv_start_index + 1]));
			m_end_char = (char)(std::stoi(std::string(argv[argv_start_index + 2])));
			m_stop_message = std::string(argv[argv_start_index + 3]);
			size_t tx_fifo_count = std::stoi(std::string(argv[argv_start_index + 4]));
			size_t rx_fifo_count = std::stoi(std::string(argv[argv_start_index + 5]));
			m_tx_fifo_paths = ParseFifoPaths(argv_start_index + 6, tx_fifo_count, argv);
			m_rx_fifo_paths = ParseFifoPaths(argv_start_index + 6 + tx_fifo_count, rx_fifo_count, argv);
		}
		catch (const std::invalid_argument& ia) {
			success = false;
			m_error_code = {-1, "invalid_argument"};
			if (m_display_messages) {
				std::string error_message = "An invalid command-line argument was passed to the client.";
				std::cerr << error_message << std::endl;
			}
		}
		catch (const std::out_of_range& oor) {
			success = false;
			m_error_code = {-1, "out_of_range_argument"};
			if (m_display_messages) {
				std::string error_message = "An out-of-range command-line argument was passed to the client.";
				std::cerr << error_message << std::endl;
			}
		}
		
		if (success) {
			if (m_display_messages) {
				std::string message = "Fifo working path is " + m_working_path;
				std::cerr << message << std::endl;
			}
			
			unsigned long timeout_seconds = 5;
			success = OpenFifos(timeout_seconds);
			if (success) {
				if (m_display_messages) {
					std::string message = "Client " + m_identifier + " initialised successfully.";
					std::cerr << message << std::endl;
				}
				m_connected = true;
			} else {
				// m_error_code already set in OpenFifos().
				if (m_display_messages) {
					std::string error_message = "Client " + m_identifier + " unable to intialise connection.";
					std::cerr << error_message << std::endl;
				}
				m_connected = false;
			}
		} else {
			m_connected = false;
		}
	} else {
		m_error_code = {-1, "ParseVarargs()_no_client_args"};
		if (m_display_messages) {
			std::string error_message = "ParseVarargs() found no client configuration arguments.";
			std::cerr << error_message << std::endl;
		}
		m_connected = false;
	}
}

SatTerm_Client::~SatTerm_Client() {
	if (IsConnected()) {
		// Some terminal emulators perform more than one fork() while starting the binary, and as such it becomes very difficult to obtain the
		// pid of the (grand)child process and waitpid() for the client to finish. Instead the server will loop on GetMessage() on the zeroth fifo
		// until GetErrorCode() returns {-1, "read()_EOF"}. We trigger that here by closing the write end of that fifo
		close(m_tx_fifo_descriptors[0]);
	}
}

size_t SatTerm_Client::GetArgStartIndex(int argc, char* argv[]) {
	size_t argument_index = 1;
	bool failed = false;
	while (std::string(argv[argument_index]) != "client_args") {
		argument_index ++;
		if (argument_index == (size_t)(argc)) {
			failed = true;
			break;
		}
	}
	if (failed) {
		return 0;
	} else {
		return argument_index + 1;
	}
}

std::vector<std::string> SatTerm_Client::ParseFifoPaths(size_t argv_start_index, size_t argv_count, char* argv[]) {
	std::vector<std::string> paths_container = {};

	for (size_t i = argv_start_index; i < argv_start_index + argv_count; i ++) {
		paths_container.emplace_back(std::string(argv[i]));
	}
	return paths_container;
}

bool SatTerm_Client::OpenFifos(unsigned long timeout_seconds) {
	bool success = false;
	success = OpenTxFifos(timeout_seconds);
	
	if (success) {
		success = OpenRxFifos(timeout_seconds);
	}
	return success;
}
