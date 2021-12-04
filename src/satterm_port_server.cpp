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

#include <iostream>                   // std::cout, std::cerr, std::endl;
#include <string>                     // std::string, std::to_string.

#include <signal.h>                   // SIGPIPE, SIG_IGN.
#include <stdio.h>                    // perror().
#include <sys/stat.h>                 // mkfifo().
#include <unistd.h>                   // close(), unlink().

#include "satterm_port.h"

Port_Server::Port_Server(std::string const& working_path, std::string const& identifier, bool display_messages, char end_char) {
	m_working_path = working_path;
	m_identifier = identifier;
	m_fifos.in.identifier = identifier + "_sin";
	m_fifos.out.identifier = identifier + "_sout";
	m_display_messages = display_messages;
	m_end_char = end_char;
	
	signal(SIGPIPE, SIG_IGN);
	
	m_fifos.in.created = CreateFifo(m_working_path + m_fifos.in.identifier);
	if (m_fifos.in.created) {
		m_fifos.out.created = CreateFifo(m_working_path + m_fifos.out.identifier);
	}
}

Port_Server::~Port_Server() {
	Close();
	if (m_fifos.in.created) {
		std::string fifo_path = m_working_path + m_fifos.in.identifier;
		int status = unlink(fifo_path.c_str());
		if ((status < 0) && m_display_messages) {
			std::string error_message = "Unable to unlink() fifo at " + fifo_path;
			perror(error_message.c_str());
		}
	}
	if (m_fifos.out.created) {
		std::string fifo_path = m_working_path + m_fifos.out.identifier;
		int status = unlink(fifo_path.c_str());
		if ((status < 0) && m_display_messages) {
			std::string error_message = "Unable to unlink() fifo at " + fifo_path;
			perror(error_message.c_str());
		}
	}
	
}

bool Port_Server::CreateFifo(std::string const& fifo_path) {
	m_error_code = {0, ""};
	bool success = false;
	
	remove(fifo_path.c_str());	// If temporary file already exists, delete it.
	
	int status = mkfifo(fifo_path.c_str(), S_IFIFO|0666);

	if (status < 0) {
		// Info on possible errors for mkfifo() - https://pubs.opengroup.org/onlinepubs/009696799/functions/mkfifo.html
		m_error_code = {errno, "mkfifo()"};
		if (m_display_messages) {
			std::string error_message = "mkfifo() error trying to create fifo " + fifo_path;
			perror(error_message.c_str());
		}
		success = false;
	} else {
		success = true;
	}
	return success;
}

bool Port_Server::Open(unsigned long timeout_seconds) {
	m_fifos.in.opened = OpenRxFifo(m_working_path + m_fifos.in.identifier, timeout_seconds);
	if (m_fifos.in.opened) {
		m_fifos.out.opened = OpenTxFifo(m_working_path + m_fifos.out.identifier, timeout_seconds);
	}
	return (m_fifos.in.opened && m_fifos.out.opened);
}

void Port_Server::Close(void) {
	if (m_fifos.out.opened) {
		close(m_fifos.out.descriptor);
		m_fifos.out.opened = false;
	}
	if (m_fifos.in.opened) {
		close(m_fifos.in.descriptor);
		m_fifos.in.opened = false;
	}
}

bool Port_Server::IsCreated(void) {
	return (m_fifos.in.created && m_fifos.out.created);
}
