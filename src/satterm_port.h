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
		Port() {}
		virtual ~Port() {}
		
		virtual bool Open(unsigned long timeout_seconds) = 0;
		virtual void Close(void) = 0;
		
		std::string GetMessage(bool capture_end_char, unsigned long timeout_seconds);
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds);
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds);
		error_descriptor GetErrorCode(void);
		bool IsOpened(void);
		
	protected:
		bool OpenRxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		bool OpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		int PollToOpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		
		error_descriptor m_error_code = {0, ""};
		bool m_display_messages = false;
		std::string m_identifier = "";
		std::string m_working_path = "";
		char m_end_char = 0;
		fifo_pair m_fifos = {{"", false, false, 0}, {"", false, false, 0}};
		std::string m_current_message = "";
		
};

class Port_Server : public Port {
	public:
		Port_Server(std::string const& working_path, std::string const& identifier, bool display_messages, char end_char);
		~Port_Server();
		
		bool Open(unsigned long timeout_seconds) override;
		void Close(void) override;
		
		bool IsCreated(void);
		
	private:
		bool CreateFifo(std::string const& fifo_path);
		
};

class Port_Client : public Port {
	public:
		Port_Client(std::string const& working_path, std::string const& identifier, bool display_messages, char end_char);
		~Port_Client();
		
		bool Open(unsigned long timeout_seconds) override;
		void Close(void) override;
		
};
