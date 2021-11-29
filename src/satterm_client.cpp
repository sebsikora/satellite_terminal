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
#include <iostream>            // std::cout, std::endl.

#include <signal.h>            // SIGPIPE, SIG_IGN.
#include <unistd.h>            // write(), read(), close().

#include "satellite_terminal.h"

// Class constructors and member function definitions for derived Client class.

// Create a client by directly specifying rx and tx fifo paths.
SatTerm_Client::SatTerm_Client(std::string const& identifier, char end_char, std::vector<std::string> rx_fifo_paths, std::vector<std::string> tx_fifo_paths, bool display_messages) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	
	signal(SIGPIPE, SIG_IGN);    // If the reader at the other end of the pipe closes prematurely, when we try and write() to the pipe
		                         // a SIGPIPE signal is generated and this process terminates.
                                 // We call signal() here to prevent the signal from being raised as-per https://stackoverflow.com/a/9036323
                                 // After each write() call we need to check the return value and if -1 check for the EPIPE error code
                                 // before/if writing again.
	m_component_type = "Client";
	m_end_char = end_char;
	m_rx_fifo_paths = rx_fifo_paths;
	m_tx_fifo_paths = tx_fifo_paths;

	Configure();
}

// Construct a client by parsing argv from the stipulated argument index (inclusive).
SatTerm_Client::SatTerm_Client(std::string const& identifier, char end_char, size_t argv_start_index, char* argv[], bool display_messages) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	
	signal(SIGPIPE, SIG_IGN);    // If the reader at the other end of the pipe closes prematurely, when we try and write() to the pipe
		                         // a SIGPIPE signal is generated and this process terminates.
                                 // We call signal() here to prevent the signal from being raised as-per https://stackoverflow.com/a/9036323
                                 // After each write() call we need to check the return value and if -1 check for the EPIPE error code
                                 // before/if writing again.
	m_component_type = "Client";
	m_end_char = end_char;

	size_t tx_fifo_count = std::stoi(std::string(argv[argv_start_index]));
	size_t rx_fifo_count = std::stoi(std::string(argv[argv_start_index + 1]));
	m_tx_fifo_paths = ParseVarargs(argv_start_index + 2, tx_fifo_count, argv);
	m_rx_fifo_paths = ParseVarargs(argv_start_index + 2 + tx_fifo_count, rx_fifo_count, argv);

	Configure();
}

SatTerm_Client::~SatTerm_Client() {
	if (IsConnected()) {
		// Some terminal emulators perform more than one fork() while starting the binary, and as such it becomes very difficult to obtain the
		// pid of the (grand)child process and waitpid() for the client to finish. Instead the server will loop on GetMessage() on the zeroth fifo
		// until GetErrorCode() returns {-1, "read()_EOF"}. We trigger that here by closing the write end of that fifo
		close(m_tx_fifo_descriptors[0]);
	}
}

std::vector<std::string> SatTerm_Client::ParseVarargs(size_t argv_start_index, size_t argv_count, char* argv[]) {
	std::vector<std::string> paths_container = {};

	for (size_t i = argv_start_index; i < argv_start_index + argv_count; i ++) {
		paths_container.emplace_back(std::string(argv[i]));
	}
	return paths_container;
}

void SatTerm_Client::Configure() {
	unsigned long timeout_seconds = 5;
	bool success = OpenFifos(timeout_seconds);

	if (success) {
		if (m_display_messages) {
			std::string message = "Client " + m_identifier + " initialised successfully.";
			std::cout << message << std::endl;
		}
		m_connected = true;
	} else {
		if (m_display_messages) {
			std::string message = "Client " + m_identifier + " unable to intialise connection.";
			std::cout << message << std::endl;
		}
		m_connected = false;
	}
}

bool SatTerm_Client::OpenFifos(unsigned long timeout_seconds) {
	bool success = false;
	success = OpenTxFifos(timeout_seconds);

	if (success) {
		success = OpenRxFifos(timeout_seconds);
	}
	return success;
}
