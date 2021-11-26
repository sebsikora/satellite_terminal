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

#include <string>
#include <vector>
#include <iostream>

#include "satellite_terminal.h"

SatTerm_Client::SatTerm_Client(std::string const& identifier, char end_char, std::vector<std::string> rx_fifo_paths, std::vector<std::string> tx_fifo_paths, bool display_messages)
 : SatTerm_Component(identifier, display_messages) {
	// Create a client by directly specifying rx and tx fifo paths.
	m_component_type = "Client";
	m_end_char = end_char;
	m_rx_fifo_paths = rx_fifo_paths;
	m_tx_fifo_paths = tx_fifo_paths;
	Configure();
}

SatTerm_Client::SatTerm_Client(std::string const& identifier, char end_char, size_t argv_start_index, char* argv[], bool display_messages)
 : SatTerm_Component(identifier, display_messages) {
	// Construct a client by parsing argv from the stipulated argument index (inclusive).
	m_component_type = "Client";
	m_end_char = end_char;
	size_t tx_fifo_count = std::stoi(std::string(argv[argv_start_index]));
	size_t rx_fifo_count = std::stoi(std::string(argv[argv_start_index + 1]));
	
	std::vector<std::string> tx_fifo_paths = {};
	size_t offset = argv_start_index + 2;
	for (size_t i = offset; i < offset + tx_fifo_count; i ++) {
		m_tx_fifo_paths.emplace_back(std::string(argv[i]));
	}
	
	std::vector<std::string> rx_fifo_paths = {};
	for (size_t i = offset + tx_fifo_count; i < offset + tx_fifo_count + rx_fifo_count; i ++) {
		m_rx_fifo_paths.emplace_back(std::string(argv[i]));
	}
	Configure();
}

void SatTerm_Client::Configure() {
	bool success = OpenFifos();
	if (success) {
		if (m_display_messages) {
			std::string message = "Client " + m_identifier + " initialised successfully.";
			std::cout << message << std::endl;
		}
		m_initialised_successfully = true;
	} else {
		if (m_display_messages) {
			std::string message = "Client " + m_identifier + " unable to open fifo(s).";
			std::cout << message << std::endl;
		}
		m_initialised_successfully = false;
	}
}

bool SatTerm_Client::OpenFifos() {
	bool success = false;
	success = OpenTxFifos();
	if (success) {
		success = OpenRxFifos();
	}
	return success;
}
