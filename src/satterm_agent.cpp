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
#include <stdexcept>                  // std::out_of_range.
#include <string>                     // std::string, std::to_string.
#include <map>                        // std::map.
#include <vector>                     // std::vector.
#include <memory>                     // std::unique_ptr.

#include "satellite_terminal.h"

SatTerm_Agent::~SatTerm_Agent() {
	// NOTE - STL containers apart from std::array make no guarantees about order of element destruction.
	//
	// If we want to destroy all the Ports 'in order' at each end, we need to force the issue rather than relying on the automatic behaviour.
	std::map<std::string, std::unique_ptr<Port>>::iterator itr = m_ports.begin();
	while (itr != m_ports.end()) {
		itr = m_ports.erase(itr);
	}
}

bool SatTerm_Agent::CreatePorts(bool is_server, std::string const& working_path, std::vector<std::string> port_identifiers,
                                bool display_messages, char end_char, std::map<std::string, std::unique_ptr<Port>>& ports) {
	bool success = true;
	for (auto const& port_identifier : port_identifiers) {
		ports.emplace(port_identifier, std::make_unique<Port>(is_server, working_path, port_identifier, display_messages, end_char));
		if (!(ports.at(port_identifier).get()->IsOpened())) {
			success = false;
			m_error_code = ports.at(port_identifier)->GetErrorCode();
			break;
		}
	}
	return success;
}

std::string SatTerm_Agent::GetMessage(bool capture_end_char, unsigned long timeout_seconds) {
	return GetMessage(m_default_port_identifier, capture_end_char, timeout_seconds);
}

std::string SatTerm_Agent::GetMessage(std::string const& port_identifier, bool capture_end_char, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	std::string received_message = "";
	try {
		received_message = m_ports.at(port_identifier)->GetMessage(capture_end_char, timeout_seconds);
		m_error_code = m_ports.at(port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_ports.at(port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "GetMessage()_OOR_port_id"};
		std::string error_message = "GetMessage() error - No port matches identifier " + port_identifier;
		std::cerr << error_message << std::endl;
	}
	return received_message;
}

std::string SatTerm_Agent::SendMessage(std::string const& message, unsigned long timeout_seconds) {
	return SendMessage(message, m_default_port_identifier, timeout_seconds);
}

std::string SatTerm_Agent::SendMessage(std::string const& message, std::string const& port_identifier, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	std::string remaining_message = "";
	try {
		remaining_message = m_ports.at(port_identifier)->SendMessage(message, timeout_seconds);
		m_error_code = m_ports.at(port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_ports.at(port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "SendMessage()_OOR_port_id"};
		std::string error_message = "SendMessage() error - No port matches identifier " + port_identifier;
		std::cerr << error_message << std::endl;
	}
	return remaining_message;
}

size_t SatTerm_Agent::SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds) {
	return SendBytes(bytes, byte_count, m_default_port_identifier, timeout_seconds);
}

size_t SatTerm_Agent::SendBytes(const char* bytes, size_t byte_count, std::string const& port_identifier, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	size_t sent_bytes = 0;
	try {
		sent_bytes = m_ports.at(port_identifier)->SendBytes(bytes, byte_count, timeout_seconds);
		m_error_code = m_ports.at(port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_ports.at(port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "SendBytes()_OOR_port_id"};
		std::string error_message = "SendBytes() error - No port matches identifier " + port_identifier;
		std::cerr << error_message << std::endl;
	}
	return sent_bytes;
}

error_descriptor SatTerm_Agent::GetErrorCode(void) {
	return m_error_code;
}

std::string SatTerm_Agent::GetStopPortIdentifier(void) {
	return m_stop_port_identifier;
}

std::string SatTerm_Agent::GetStopMessage(void) {
	return m_stop_message;
}

bool SatTerm_Agent::IsConnected(void) {
	return m_connected;
}

void SatTerm_Agent::SetConnectedFlag(bool is_connected) {
	m_connected = is_connected;
}
