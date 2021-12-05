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
#include <string>                     // std::string, std::stoi.
#include <map>                        // std::map.
#include <vector>                     // std::vector.

#include "satellite_terminal.h"

SatTerm_Client::SatTerm_Client(std::string const& identifier, int argc, char* argv[], bool display_messages) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	
	size_t port_count = 0;
	std::vector<std::string> port_identifiers = {};
	
	size_t argv_start_index = GetArgStartIndex("client_args", argc, argv);

	bool success = false;
	if (argv_start_index != 0) {
		
		m_working_path = std::string(argv[argv_start_index]);
		m_end_char = (char)(std::stoi(std::string(argv[argv_start_index + 1])));
		m_stop_message = std::string(argv[argv_start_index + 2]);
		port_count = std::stoi(std::string(argv[argv_start_index + 3]));
		port_identifiers = ParseFifoPaths(argv_start_index + 4, port_count, argv);

		m_default_port_identifier = port_identifiers[0];
		
		if (m_display_messages) {
			std::string message = "Client working path is " + m_working_path;
			std::cerr << message << std::endl;
		}
		
		success = CreatePorts(false, m_working_path, port_identifiers, m_display_messages, m_end_char, m_ports);
		
		if (success) {
			if (m_display_messages) {
				std::string message = "Client " + m_identifier + " successfully initialised connection.";
				std::cerr << message << std::endl;
			}
		} else {
			if (m_display_messages) {
				std::string error_message = "Client " + m_identifier + " unable to intialise connection.";
				std::cerr << error_message << std::endl;
			}
		}
	} else {
		m_error_code = {-1, "GetArgStartIndex()_invalid_args"};
		if (m_display_messages) {
			std::string error_message = "GetArgStartIndex() found invalid client configuration arguments.";
			std::cerr << error_message << std::endl;
		}
	}
	SetConnectedFlag(success);
}

SatTerm_Client::~SatTerm_Client() {
	if (IsConnected()) {
		SendMessage(m_stop_message, m_stop_port_identifier);
	}
}

size_t SatTerm_Client::GetArgStartIndex(std::string const& arg_start_delimiter, int argc, char* argv[]) {
	bool success = true;
	size_t argument_index = 1;
	if (argc > 1) {
		while (std::string(argv[argument_index]) != arg_start_delimiter) {
			argument_index ++;
			if (argument_index == (size_t)(argc)) {
				success = false;
				break;
			}
		}
	} else {
		success = false;
	}
	if (success) {
		return argument_index + 1;
	} else {
		return 0;
	}
}

std::vector<std::string> SatTerm_Client::ParseFifoPaths(size_t argv_start_index, size_t argv_count, char* argv[]) {
	std::vector<std::string> paths_container = {};

	for (size_t i = argv_start_index; i < argv_start_index + argv_count; i ++) {
		paths_container.emplace_back(std::string(argv[i]));
	}
	return paths_container;
}
