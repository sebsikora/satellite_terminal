#include <iostream>
#include <fstream>                    // std::ifstream.
#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include "satellite_terminal.h"

bool SatTerm_Agent::CreatePorts(bool is_server, bool is_in_port, std::string const& working_path, std::vector<std::string> port_identifiers,
                                bool display_messages, char end_char, std::map<std::string, std::unique_ptr<Port>>& ports) {
	bool success = true;
	for (auto const& port_identifier : port_identifiers) {
		ports.emplace(port_identifier, std::make_unique<Port>(is_server, is_in_port, working_path, port_identifier, display_messages, end_char));
		if (is_server) {
			if (!(ports.at(port_identifier)->IsCreated())) {
				success = false;
				m_error_code = ports.at(port_identifier)->GetErrorCode();
				break;
			}
		}
	}
	return success;
}

bool SatTerm_Agent::OpenPorts(std::map<std::string, std::unique_ptr<Port>>& ports, unsigned long timeout_seconds) {
	bool success = true;
	for (auto const& current_port : ports) {
		current_port.second->Open(timeout_seconds);
		if (!(current_port.second->IsOpened())) {
			success = false;
			m_error_code = current_port.second->GetErrorCode();
			break;
		}
	}
	return success;
}

std::string SatTerm_Agent::GetMessage(bool capture_end_char, unsigned long timeout_seconds) {
	return GetMessage(m_default_port.rx, capture_end_char, timeout_seconds);
}

std::string SatTerm_Agent::GetMessage(std::string const& in_port_identifier, bool capture_end_char, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	std::string received_message = "";
	try {
		received_message = m_in_ports.at(in_port_identifier)->GetMessage(capture_end_char, timeout_seconds);
		m_error_code = m_in_ports.at(in_port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_in_ports.at(in_port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "GetMessage()_OOR_port_id"};
		std::string error_message = "GetMessage() error - No port matches identifier " + in_port_identifier;
		std::cerr << error_message << std::endl;
	}
	return received_message;
}

std::string SatTerm_Agent::SendMessage(std::string const& message, unsigned long timeout_seconds) {
	return SendMessage(message, m_default_port.tx, timeout_seconds);
}

std::string SatTerm_Agent::SendMessage(std::string const& message, std::string const& out_port_identifier, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	std::string remaining_message = "";
	try {
		remaining_message = m_out_ports.at(out_port_identifier)->SendMessage(message, timeout_seconds);
		m_error_code = m_out_ports.at(out_port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_out_ports.at(out_port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "SendMessage()_OOR_port_id"};
		std::string error_message = "SendMessage() error - No port matches identifier " + out_port_identifier;
		std::cerr << error_message << std::endl;
	}
	return remaining_message;
}

size_t SatTerm_Agent::SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds) {
	return SendBytes(bytes, byte_count, m_default_port.tx, timeout_seconds);
}

size_t SatTerm_Agent::SendBytes(const char* bytes, size_t byte_count, std::string const& out_port_identifier, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	size_t sent_bytes = 0;
	try {
		sent_bytes = m_out_ports.at(out_port_identifier)->SendBytes(bytes, byte_count, timeout_seconds);
		m_error_code = m_out_ports.at(out_port_identifier)->GetErrorCode();
		if (m_error_code.err_no != 0) {
			SetConnectedFlag(m_out_ports.at(out_port_identifier)->IsOpened());
		}
	}
	
	catch (const std::out_of_range& oor) {
		m_error_code = {-1, "SendBytes()_OOR_port_id"};
		std::string error_message = "SendBytes() error - No port matches identifier " + out_port_identifier;
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
