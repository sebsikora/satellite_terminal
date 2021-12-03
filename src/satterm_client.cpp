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
		success = true;
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
		
		CreateClientPorts(m_working_path, port_identifiers, m_display_messages, m_end_char, m_ports);
		
		unsigned long timeout_seconds = 5;
		
		if (success) {
			success = OpenPorts(m_ports, timeout_seconds);
		}
		
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
	// If shutdown is occurring, the server will poll m_default_port.in until it gets EOF, then everything will be closed and unlinked at the
	// server end. To make sure that unlink() deletes the files, we close any that are open here, saving m_default_port.out at this end for last.
	for (const auto& port : m_ports) {
		if (port.first != m_default_port_identifier) {
			port.second->Close();
		}
	}
	m_ports.at(m_default_port_identifier)->Close();
	// There should not be any more to do. Pointers in m_in_ports and m_out_ports are stored as unique_ptr<Port>, so when the maps
	// are destroyed now the destructors for the Ports should be called automatically. In these the fifos will be unlink()ed if m_is_server is set.
}

size_t SatTerm_Client::GetArgStartIndex(std::string const& arg_start_delimiter, int argc, char* argv[]) {
	size_t argument_index = 1;
	bool failed = false;
	while (std::string(argv[argument_index]) != arg_start_delimiter) {
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

void SatTerm_Client::CreateClientPorts(std::string const& working_path, std::vector<std::string> port_identifiers,
                                       bool display_messages, char end_char, std::map<std::string, std::unique_ptr<Port>>& ports) {
	for (auto const& port_identifier : port_identifiers) {
		ports.emplace(port_identifier, std::make_unique<Port_Client>(working_path, port_identifier, display_messages, end_char));
	}
}
