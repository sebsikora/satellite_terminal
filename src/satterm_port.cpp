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

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <ctime>                      // time().

#include <stdio.h>                    // perror().
#include <fcntl.h>                    // open() and O_RDONLY, O_WRONLY, etc.
#include <unistd.h>                   // write(), read(), close(), unlink().
#include <sys/stat.h>                 // mkfifo().
#include <errno.h>                    // errno.
#include <signal.h>                   // SIGPIPE, SIG_IGN.

#include "satterm_port.h"

Port::Port(bool is_server, bool is_in_port, std::string const& working_path, std::string const& identifier, bool display_messages, char end_char) {
	m_is_server = is_server;
	m_is_in_port = is_in_port;
	m_working_path = working_path;
	m_identifier = identifier;
	m_display_messages = display_messages;
	m_end_char = end_char;
	
	signal(SIGPIPE, SIG_IGN);
	
	if (m_is_server) {
		Create();
	}
}

Port::~Port() {
	if (m_opened) {
		close(m_fifo_descriptor);
	}
	if ((m_is_server) && (m_created)) {
		Close();
		// Unlink.
		std::string fifo_path = m_working_path + m_identifier;
		int status = unlink(fifo_path.c_str());
		if ((status < 0) && m_display_messages) {
			std::string error_message = "Unable to unlink fifo at path " + fifo_path;
			perror(error_message.c_str());
		}
	}
}

void Port::Close(void) {
	close(m_fifo_descriptor);
	m_opened = false;
}

void Port::Create(void) {
	m_error_code = {0, ""};
	
	std::string fifo_path = m_working_path + m_identifier;
	
	remove(fifo_path.c_str());	// If temporary file already exists, delete it.
	
	int status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);

	if (status < 0) {
		// Info on possible errors for mkfifo() - https://pubs.opengroup.org/onlinepubs/009696799/functions/mkfifo.html
		m_error_code = {errno, "mkfifo()"};
		if (m_display_messages) {
			std::string error_message = "Server mkfifo() error trying to create fifo at path " + fifo_path;
			perror(error_message.c_str());
		}
		m_created = false;
	} else {
		m_created = true;
	}
}

void Port::Open(unsigned long timeout_seconds) {
	if (m_is_in_port) {
		m_opened = OpenRxFifo(m_working_path + m_identifier, timeout_seconds);
	} else {
		m_opened = OpenTxFifo(m_working_path + m_identifier, timeout_seconds);
	}
}

bool Port::OpenRxFifo(std::string const& fifo_path, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	int fifo = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
	
	bool success = false;
	if (!(fifo < 0)) {
		m_fifo_descriptor = fifo;
		m_current_message = "";
		
		std::string init_message = GetMessage(false, timeout_seconds);
		
		if (init_message == "init") {
			if (m_display_messages) {
				std::string message = "In Port " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo);
				std::cerr << message << std::endl;
			}
			success = true;
		} else {
			m_fifo_descriptor = 0;
			if (m_error_code == error_descriptor{-1, "GetMessage()_tx_unconn_timeout"}) {
				if (m_display_messages) {
					std::string error_message =  "In Port " + m_identifier + " opened fifo " + fifo_path + " for reading on descriptor " + std::to_string(fifo) + " but timed-out waiting for an init message.";
					std::cerr << error_message << std::endl;
				}
			}
			success = false;
		}
	} else {
		m_error_code = {errno, "open()_rx"};
		if (m_display_messages) {
			std::string error_message = "In Port " + m_identifier + " unable to open fifo " + fifo_path + " for reading.";
			perror(error_message.c_str());
		}
		success = false;
	}
	return success;
}

bool Port::OpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	int fifo = PollToOpenTxFifo(fifo_path, timeout_seconds); 

	bool success = false;
	if (fifo >= 0) {
		if (m_display_messages) {
			std::string message = "Out Port " + m_identifier + " opened fifo " + fifo_path + " for writing on descriptor " + std::to_string(fifo);
			std::cerr << message << std::endl;
		}
		
		m_fifo_descriptor = fifo;
		
		std::string init_message = "init";
		init_message = SendMessage(init_message, timeout_seconds);
		
		if (GetErrorCode().err_no != 0) {
			m_fifo_descriptor = 0;
			success = false;
		} else {
			success = true;
		}
	} else {
		success = false;
	}
	return success;
}

int Port::PollToOpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	unsigned long start_time = time(0);
	int fifo = -1;
	
	bool finished = false;
	while (!finished) {
		
		fifo = open(fifo_path.c_str(), O_WRONLY | O_NONBLOCK);
		
		if (fifo >= 0) {
			finished = true;
		} else {
			switch (errno) {
				case ENXIO:
					finished = ((time(0) - start_time) > timeout_seconds);
					if (finished) {
						m_error_code = {-1, "PollToOpenTxFifo()_rx_conn_timeout"};
					}
					break;
				default:
					m_error_code = {errno, "open()_tx"};
					if (m_display_messages) {
						std::string error_message = "Out Port " + m_identifier + " unable to open fifo " + fifo_path + " for writing.";
						perror(error_message.c_str());
					}
					finished = true;
			}
		}
	}
	return fifo;
}

std::string Port::SendMessage(std::string const& message, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	std::string working_message = message;
	working_message.push_back(m_end_char);
	size_t working_message_length = working_message.size();
	
	size_t bytes_sent = SendBytes(working_message.c_str(), working_message_length, timeout_seconds);
	
	if (bytes_sent == working_message_length) {                     // Sent whole working_message including m_end_char.
		return "";
	} else if (bytes_sent == (working_message_length - 1)) {        // Sent whole message but did not send m_end_char.
		return "";
	} else {                                                        // Sent part of message.
		return message.substr(bytes_sent, std::string::npos);
	}
}

size_t Port::SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	if (!m_is_in_port) {
		unsigned long start_time = time(0);
		bool finished = false;
		size_t bytes_remaining = byte_count;
		size_t offset = 0;

		while (!finished) {
			
			int status = write(m_fifo_descriptor, bytes + offset, bytes_remaining);
			
			if (status >= 0) {
				if ((size_t)(status) == bytes_remaining) {
					finished = true;
				} else {
					offset += (size_t)(status);
				}
				bytes_remaining -= (size_t)(status);
			} else {
				switch (errno) {
					case EAGAIN:                    // Erro - thread would block (reader currently reading, etc). Try again unless timeout.
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
							std::string error_message = "Error on write() for Out_Port " + m_identifier;
							perror(error_message.c_str());
						}
						m_opened = false;
						finished = true;
				}
			}
		}
		return byte_count - bytes_remaining;
	} else {
		m_error_code = {-1, "SendBytes()_in_port"};
		if (m_display_messages) {
			std::string error_message = "In Port " + m_identifier + " unable to SendBytes().";
			std::cerr << error_message << std::endl;
		}
		return 0;
	}
}

std::string Port::GetMessage(bool capture_end_char, unsigned long timeout_seconds) {
	m_error_code = {0, ""};
	
	if (m_is_in_port) {
		unsigned long start_time = 0;
		if (timeout_seconds > 0) {
			start_time = time(0);
		}
		
		bool finished = false;
		bool end_char_received = false;
		char char_in;
		std::string message;
		
		while (!finished) {
			
			int status = read(m_fifo_descriptor, &char_in, 1);
			
			if (status > 0) {                       // read() read-in a character to the char buffer.
				if (char_in != m_end_char) {
					m_current_message.push_back(char_in);
				} else {
					if (capture_end_char) {
						m_current_message.push_back(char_in);
					}
					message = m_current_message;
					m_current_message = "";
					end_char_received = true;
					finished = true;
				}
				
			} else if (status == 0) {                                            // EOF. read() will return this if no process has the pipe open for writing.
					if (!m_opened) {
						finished = ((time(0) - start_time) > timeout_seconds);   // If the Component has not finished initialising, assume that the partner
						if (finished) {                                          // component hasn't opened the fifo for writing yet so continue to poll   
							m_error_code = {-1, "GetMessage()_tx_unconn_timeout"};         // until timeout.
						}
					} else {                                                     // If the Component is initialised, the partner component no-longer has the
						m_error_code = {-1, "read()_EOF"};                       // fifo open for writing (has become disconnected).
						if (m_display_messages) {
							std::string error_message = "EOF error on GetMessage() for In Port " + m_identifier + " suggests counterpart terminated.";
							std::cerr << error_message << std::endl;
						}
						m_opened = false;
						finished = true;
					}
			
			} else if (status < 0) {                // read() indicates an error.
				switch (errno) {                    // See under errors here - https://pubs.opengroup.org/onlinepubs/009604599/functions/read.html
					case EAGAIN:					// Non-blocking read on empty fifo with connected writer will return -1 with error EAGAIN,
						finished = ((time(0) - start_time) > timeout_seconds);                           // so we continue to poll unless timeout.
						if (finished && (timeout_seconds > 0)) {        // Only set m_error_code to EAGAIN if we have been waiting on a timeout.
							m_error_code = {errno, "GetMessage()_tx_conn_timeout"};
						}
						break;
					default:                        // Trap all other read() errors here.
						m_error_code = {errno, "read()"};
						if (m_display_messages) {
							std::string error_message = "Error on read() for In Port " + m_identifier;
							perror(error_message.c_str());
						}
						m_opened = false;
						finished = true;
				}
			}
		}
		if (end_char_received) {
			return message;
		} else {
			return "";
		}
	} else {
		m_error_code = {-1, "GetMessage()_out_port"};
		if (m_display_messages) {
			std::string error_message = "Out Port " + m_identifier + " unable to GetMessage().";
			std::cerr << error_message << std::endl;
		}
		return "";
	}
}

error_descriptor Port::GetErrorCode(void) {
	return m_error_code;
}

bool Port::IsCreated(void) {
	return m_created;
}

bool Port::IsOpened(void) {
	return m_opened;
}

bool Port::IsInPort(void) {
	return m_is_in_port;
}
