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

#include <string>                    // std::string.
#include <vector>                    // std::vector.

#include "satterm_struct.h"

class Port {
	public:
		Port(bool is_server, std::string const& working_path, std::string const& identifier, bool display_messages, char end_char);
		~Port();
		
		bool IsOpened(void);
		std::string GetMessage(bool capture_end_char, unsigned long timeout_seconds);
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds);
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds);
		error_descriptor GetErrorCode(void);
		
	protected:
		bool CreateFifo(std::string const& fifo_path);
		bool OpenFifos(bool is_server, unsigned long timeout_seconds);
		bool OpenRxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		bool OpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		int PollToOpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		void CloseFifos(void);
		void UnlinkInFifo(void);
		
		error_descriptor m_error_code = {0, ""};
		bool m_display_messages = false;
		std::string m_identifier = "";
		std::string m_working_path = "";
		char m_end_char = 0;
		fifo_pair m_fifos = {{"", false, false, 0}, {"", false, false, 0}};
		std::string m_current_message = "";
		
};
