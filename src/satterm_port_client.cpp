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

#include <signal.h>                   // SIGPIPE, SIG_IGN.
#include <unistd.h>                   // close(), unlink().

#include "satterm_port.h"

Port_Client::Port_Client(std::string const& working_path, std::string const& identifier, bool display_messages, char end_char) {
	m_working_path = working_path;
	m_identifier = identifier;
	m_fifos.in.identifier = identifier + "_sout";
	m_fifos.out.identifier = identifier + "_sin";
	m_display_messages = display_messages;
	m_end_char = end_char;
	
	signal(SIGPIPE, SIG_IGN);
	
}

Port_Client::~Port_Client() {
	Close();
}

bool Port_Client::Open(unsigned long timeout_seconds) {
	m_fifos.out.opened = OpenTxFifo(m_working_path + m_fifos.out.identifier, timeout_seconds);
	if (m_fifos.out.opened) {
		m_fifos.in.opened = OpenRxFifo(m_working_path + m_fifos.in.identifier, timeout_seconds);
	}
	return (m_fifos.out.opened && m_fifos.in.opened);
}

void Port_Client::Close(void) {
	if (m_fifos.in.opened) {
		close(m_fifos.in.descriptor);
		m_fifos.in.opened = false;
	}
	if (m_fifos.out.opened) {
		close(m_fifos.out.descriptor);
		m_fifos.out.opened = false;
	}
}

