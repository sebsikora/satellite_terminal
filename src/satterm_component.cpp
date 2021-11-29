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

#include <stdio.h>            // perror().
#include <fcntl.h>            // open() and O_RDONLY, O_WRONLY.
#include <unistd.h>           // write(), read().
#include <errno.h>            // errno.

#include <string>             // std::string.
#include <vector>             // std::vector.
#include <iostream>           // std::cout, std::cerr, std::endl.
#include <ctime>              // time().

#include "satellite_terminal.h"

// Class member function definitions for abstract base Component class.

bool SatTerm_Component::IsConnected() {
	return m_connected;
}

bool SatTerm_Component::OpenRxFifos(unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	bool success = false;
	
	for (size_t rx_fifo_index = 0; rx_fifo_index < m_rx_fifo_paths.size(); rx_fifo_index ++) {
		std::string fifo_path = m_rx_fifo_paths[rx_fifo_index];
		
		int fifo = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
		
		if (!(fifo < 0)) {
			m_rx_fifo_descriptors.emplace_back(fifo);
			m_current_messages.emplace_back("");
			
			std::string init_message = GetMessage(rx_fifo_index, false, timeout_seconds);
			
			if (init_message == "init") {
				success = true;
				if (m_display_messages) {
					std::string message = m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo);
					std::cout << message << std::endl;
				}
			} else {
				if (m_error_code == error_descriptor{-1, "GetMessage()_tx_unconn_timeout"}) {
					if (m_display_messages) {
						std::string error_message =  m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo) + " but timed-out waiting for an init message.";
						std::cout << error_message << std::endl;
					}
				}
				success = false;
				break;
			}
		} else {
			m_error_code = {errno, "open()_rx"};
			if (m_display_messages) {
				std::string error_message = m_component_type + " " + m_identifier + " unable to open fifo " + fifo_path + " for reading.";
				perror(error_message.c_str());
			}
			success = false;
			break;
		}
		rx_fifo_index ++;
	}
	return success;
}

bool SatTerm_Component::OpenTxFifos(unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	bool success = true;
	unsigned long start_time = time(0);
	int fifo;
	
	for (size_t fifo_index = 0; fifo_index < m_tx_fifo_paths.size(); fifo_index ++) {
		std::string fifo_path = m_tx_fifo_paths[fifo_index];
		
		fifo = PollOpenForTx(fifo_path, start_time, timeout_seconds); 
		
		if (fifo >= 0) {
			if (m_display_messages) {
				std::string message = m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for writing on descriptor " + std::to_string(fifo);
				std::cout << message << std::endl;
			}
			m_tx_fifo_descriptors.emplace_back(fifo);
			std::string init_message = "init";
			
			size_t chars_sent = SendMessage(init_message, fifo_index);
			
			if (chars_sent < init_message.size() + 1) {
				success = false;
				break;
			}
		} else {
			success = false;
			break;
		}
	}
	return success;
}

int SatTerm_Component::PollOpenForTx(std::string const& fifo_path, unsigned long start_time, unsigned long timeout_seconds) {
	int fifo = -1;
	bool finished = false;
	m_error_code = {0, ""};
	
	while (!finished) {
		
		fifo = open(fifo_path.c_str(), O_WRONLY | O_NONBLOCK);
		
		if (fifo >= 0) {
			finished = true;
		} else {
			switch (errno) {
				case ENXIO:
					finished = ((time(0) - start_time) > timeout_seconds);
					if (finished) {
						m_error_code = {-1, "PollForOpenTx()_rx_conn_timeout"};
					}
					break;
				default:
					m_error_code = {errno, "open()_tx"};
					if (m_display_messages) {
						std::string error_message = m_component_type + " " + m_identifier + " unable to open fifo " + fifo_path + " for writing.";
						perror(error_message.c_str());
					}
					finished = true;
			}
		}
	}
	return fifo;
}

std::string SatTerm_Component::GetMessage(size_t rx_fifo_index, bool capture_end_char, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	char char_in;
	bool end_char_received = false;
	std::string message;
	bool finished = false;
	
	unsigned long start_time = 0;
	if (timeout_seconds > 0) {
		start_time = time(0);
	}
	
	while (!finished) {
		
		int status = read(m_rx_fifo_descriptors[rx_fifo_index], &char_in, 1);
		
		if (status > 0) {                       // read() read-in a character to the char buffer.
			if (char_in != m_end_char) {
				m_current_messages[rx_fifo_index].push_back(char_in);
			} else {
				if (capture_end_char) {
					m_current_messages[rx_fifo_index].push_back(char_in);
				}
				end_char_received = true;
				message = m_current_messages[rx_fifo_index];
				m_current_messages[rx_fifo_index] = "";
				break;
			}
		
		} else if (status == 0) {                                            // EOF. read() will return this if no process has the pipe open for writing.
			    if (!m_connected) {
					finished = ((time(0) - start_time) > timeout_seconds);   // If the Component has not finished initialising, assume that the partner
					if (finished) {                                          // component hasn't opened the fifo for writing yet so continue to poll   
						m_error_code = {-1, "GetMessage()_tx_unconn_timeout"};         // until timeout.
					}
				} else {                                                     // If the Component is initialised, the partner component no-longer has the
					m_error_code = {-1, "read()_EOF"};                       // fifo open for writing (has become disconnected).
					if (m_display_messages) {
						std::string error_message = "EOF on read() to fifo index " + std::to_string(rx_fifo_index) + " suggests counterpart terminated.";
						std::cout << error_message << std::endl;
					}
					m_connected = false;
					finished = true;
				}
		
		} else if (status < 0) {                // read() indicates an error.
			switch (errno) {                    // See under errors here - https://pubs.opengroup.org/onlinepubs/009604599/functions/read.html
				case EAGAIN:					// Non-blocking read on empty fifo with connected writer will return -1 with error EAGAIN,
					finished = ((time(0) - start_time) > timeout_seconds);                                      // so we continue to poll.
					if (finished && (timeout_seconds > 0)) {
						m_error_code = {-1, "GetMessage()_tx_conn_timeout"};
					}
					break;
				default:                        // Trap all other read() errors here.
					m_error_code = {errno, "read()"};
					if (m_display_messages) {
						std::string error_message = "Error on read() to fifo index " + std::to_string(rx_fifo_index);
						perror(error_message.c_str());
					}
					m_connected = false;
					finished = true;
			}
		}
	}
	
	if (end_char_received) {
		return message;
	} else {
		return "";
	}
}

size_t SatTerm_Component::SendMessage(std::string const& message, size_t tx_fifo_index, unsigned long timeout_seconds) {
	std::string msg = message;
	msg.push_back(m_end_char);
	size_t message_length = msg.size();
	
	size_t success = SendBytes(msg.c_str(), message_length, tx_fifo_index, timeout_seconds);
	
	return success;
}

size_t SatTerm_Component::SendBytes(const char* bytes, size_t byte_count, size_t tx_fifo_index, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	unsigned long start_time = time(0);
	bool finished = false;
	
	size_t offset = 0;
	size_t bytes_remaining = byte_count;
	while (!finished) {
		
		int status = write(m_tx_fifo_descriptors[tx_fifo_index], bytes + offset, bytes_remaining);
		
		if (status >= 0) {
			if ((size_t)(status) == bytes_remaining) {
				finished = true;
			} else {
				offset += (size_t)(status);
			}
			bytes_remaining -= (size_t)(status);
		
		} else {
			switch (errno) {
				case EAGAIN:
					finished = ((time(0) - start_time) > timeout_seconds);
					if ((finished) && (timeout_seconds == 0)) {
						m_error_code = {errno, "write()_thread_block"};
					} else if ((finished) && (timeout_seconds > 0)) {
						m_error_code = {errno, "write()_thread_block_timeout"};
					}
					break;
				default:                        // Trap all other write() errors here.
					m_error_code = {errno, "write()"};
					if (m_display_messages) {
						std::string error_message = "Error on write() to fifo " + m_tx_fifo_paths[tx_fifo_index];
						perror(error_message.c_str());
					}
					m_connected = false;
					finished = true;
			}
		}
	}
	return byte_count - bytes_remaining;
}

size_t SatTerm_Component::GetTxFifoCount(void) {
	return m_tx_fifo_descriptors.size();
}

size_t SatTerm_Component::GetRxFifoCount(void) {
	return m_rx_fifo_descriptors.size();
}

error_descriptor SatTerm_Component::GetErrorCode() {
	return m_error_code;
}

std::string SatTerm_Component::GetStopMessage() {
	return m_stop_message;
}
