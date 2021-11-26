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
#include <fcntl.h>            // open() and O_RDONLY, O_WRONLY, etc.
#include <unistd.h>           // write(), read(), sleep(), fork(), execl(), close(), unlink().
#include <errno.h>            // errno.
#include <signal.h>           // SIGPIPE, SIG_IGN.

#include <string>             // std::string.
#include <vector>             // std::vector.
#include <iostream>           // std::cout, std::cerr, std::endl.
#include <ctime>              // time(), mktime().

#include "satellite_terminal.h"

SatTerm_Component::SatTerm_Component(std::string const& identifier, bool display_messages) {
	m_identifier = identifier;
	m_display_messages = display_messages;
	signal(SIGPIPE, SIG_IGN);    // If the reader at the other end of the pipe closes prematurely, when we try and write() to the pipe
		                         // a SIGPIPE signal is generated and this process terminates.
                                 // We call signal() here to prevent the signal from being raised as-per https://stackoverflow.com/a/9036323
                                 // After each write() call we need to check the return value and if -1 check for the EPIPE error code
                                 // before/if writing again.
}

bool SatTerm_Component::IsInitialised() {
	return m_initialised_successfully;
}

bool SatTerm_Component::OpenRxFifos() {
	m_error_code = 0;
	bool success = false;
	for (const auto& fifo_path : m_rx_fifo_paths) {
		unsigned long timeout_seconds = 5;
		unsigned long start_time = time(0);
		int fifo = -1;
		while ((fifo < 0) && (time(0) - start_time < timeout_seconds)) {
			fifo = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
		}
		if (!(fifo < 0)) {
			std::string init_message = "";
			char char_in;
			bool irrecoverable_error = false;
			while (time(0) - start_time < timeout_seconds) {
				int status = read(fifo, &char_in, 1);
				if (status > 0) {
					if (char_in != m_end_char) {
						init_message.push_back(char_in);
					} else {
						if (init_message == "init") {
							success = true;
							break;
						}
					}
				} else if (status == 0) {
					// No characters returned but no error condition. Ostensibly read() should not return this when fifo read end opened in non-blocking mode.
				} else if (status < 0) {
					switch (errno) {
						case EAGAIN:					// Non-blocking read on empty fifo will return -1 with error EAGAIN, so we don't trap these
							break;                      // otherwise this will trip almost all the time - see under errors here https://pubs.opengroup.org/onlinepubs/009604599/functions/read.html
						default:
							m_error_code = errno;
							if (m_display_messages) {
								std::string error_message = "Error on read() to fifo at " + fifo_path;
								perror(error_message.c_str());
							}
							irrecoverable_error = true;
					}
					if (irrecoverable_error) {
						break;
					}
				}
			}
			if (success) {	
				m_rx_fifo_descriptors.emplace_back(fifo);
				m_current_messages.emplace_back("");
				if (m_display_messages) {
					std::string message = m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo);
					std::cout << message << std::endl;
				}
			} else if (!irrecoverable_error) {
				if (m_display_messages) {
					std::string error_message =  m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo) + " but timed-out waiting for an init message.";
					std::cout << error_message << std::endl;
				}
			}
		} else {
			m_error_code = errno;
			if (m_display_messages) {
				std::string error_message = m_component_type + " " + m_identifier + " unable to open fifo " + fifo_path + " for reading.";
				perror(error_message.c_str());
			}
			success = false;
			break;
		}
	}
	return success;
}

bool SatTerm_Component::OpenTxFifos() {
	m_error_code = 0;
	bool success = false;
	for (const auto& fifo_path : m_tx_fifo_paths) {
		int fifo = open(fifo_path.c_str(), O_WRONLY);
		if (!(fifo < 0)) {
			if (m_display_messages) {
				std::string message = m_component_type + " " + m_identifier + " opened fifo " + fifo_path + " for writing on descriptor " + std::to_string(fifo);
				std::cout << message << std::endl;
			}
			std::string init_message = "init" + std::string(1, m_end_char);
			int status = write(fifo, init_message.c_str(), init_message.size());
			if (status < 0) {
				m_error_code = errno;
				if (m_display_messages) {
					std::string error_message = "Error on write() to fifo " + fifo_path;
					perror(error_message.c_str());
				}
				success = false;
				break;
			} else {
				m_tx_fifo_descriptors.emplace_back(fifo);
				success = true;
			}
		} else {
			m_error_code = errno;
			if (m_display_messages) {
				std::string error_message = m_component_type + " " + m_identifier + " unable to open fifo " + fifo_path + " for writing.";
				perror(error_message.c_str());
			}
			success = false;
			break;
		}
	}
	return success;
}

std::string SatTerm_Component::GetMessage(bool capture_end_char, int rx_fifo_index) {
	m_error_code = 0;
	char char_in;
	bool end_char_received = false;
	std::string message;
	bool irrecoverable_error = false;
	while (true) {
		int status = read(m_rx_fifo_descriptors[rx_fifo_index], &char_in, 1);
		if (status > 0) {
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
		} else if (status == 0) {
			// No characters returned but no error condition. Ostensibly read() should not return this when fifo read end opened in non-blocking mode.
		} else if (status < 0) {
			switch (errno) {
				case EAGAIN:					// Non-blocking read on empty fifo will return -1 with error EAGAIN, so we don't trap these
					break;                      // otherwise this will trip almost all the time - see under errors here https://pubs.opengroup.org/onlinepubs/009604599/functions/read.html
				default:
					m_error_code = errno;
					if (m_display_messages) {
						std::string error_message = "Error on read() to fifo index " + std::to_string(rx_fifo_index);
						perror(error_message.c_str());
					}
					irrecoverable_error = true;
			}
			if (irrecoverable_error) {
				break;
			}
		}
	}
	if (end_char_received) {
		return message;
	} else {
		return "";
	}
}

int SatTerm_Component::SendMessage(std::string message, int tx_fifo_index) {
	std::string msg = message;
	msg.push_back(m_end_char);
	int message_length = msg.size();
	int success = SendBytes(msg.c_str(), message_length, tx_fifo_index);
	return success;
}

int SatTerm_Component::SendBytes(const char* bytes, int byte_count, int tx_fifo_index) {
	m_error_code = 0;
	int success = write(m_tx_fifo_descriptors[tx_fifo_index], bytes, byte_count);
	if (success < 0) {	// Now that we are blocking the SIGPIPE signal, we need to check if any errors are raied
							// after each call to write().
		m_error_code = errno;
		if (m_display_messages) {
			std::string error_message = "Error on write() to fifo index " + std::to_string(tx_fifo_index);
			perror(error_message.c_str());
		}
	}
	return success;
}

int SatTerm_Component::GetTxFifoCount(void) {
	return m_tx_fifo_descriptors.size();
}

int SatTerm_Component::GetRxFifoCount(void) {
	return m_rx_fifo_descriptors.size();
}

int SatTerm_Component::GetErrorCode() {
	return m_error_code;
}
