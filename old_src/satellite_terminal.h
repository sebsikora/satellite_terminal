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

#include <string>                    // std::string.
#include <vector>                    // std::vector.

struct error_descriptor {
	int err_no;
	std::string function;
	error_descriptor& operator=(error_descriptor const& rhs) {
		err_no = rhs.err_no;
		function = rhs.function;
		return *this;
	}
	bool operator==(error_descriptor const& rhs) const {
		return ((this->err_no == rhs.err_no) && (this->function == rhs.function));
	}
	bool operator!=(error_descriptor const& rhs) const {
		return ((this->err_no != rhs.err_no) || (this->function != rhs.function));
	}
};

// Component base class. Functionality common to both server and client classes.
class SatTerm_Component {
	public:
		SatTerm_Component () {}				// Abstract base class constructor.
		virtual ~SatTerm_Component () {}	// virtual destructor for abstract base class.
		std::string GetMessage(size_t rx_fifo_index = 0, bool capture_end_char = false, unsigned long timeout_seconds = 0);
		std::string SendMessage(std::string const& message, size_t tx_fifo_index = 0, unsigned long timeout_seconds = 10);
		size_t SendBytes(const char* bytes, size_t byte_count, size_t tx_fifo_index = 0, unsigned long timeout_seconds = 10);
		bool IsConnected(void);
		size_t GetTxFifoCount(void);
		size_t GetRxFifoCount(void);
		error_descriptor GetErrorCode(void);
		std::string GetStopMessage(void);
		int GetStopFifoIndex(void);
		
	protected:
		bool OpenRxFifos(unsigned long timeout_seconds);
		bool OpenTxFifos(unsigned long timeout_seconds);
		
		bool m_display_messages = false;
		std::string m_component_type = "";
		std::string m_identifier = "";
		char m_end_char = 0;
		int m_stop_fifo_index = 0;
		std::string m_stop_message = "";
		std::vector<int> m_tx_fifo_descriptors = {};
		std::vector<int> m_rx_fifo_descriptors = {};
		std::string m_working_path = "";
		std::vector<std::string> m_rx_fifo_paths = {};
		std::vector<std::string> m_tx_fifo_paths = {};
		bool m_connected = false;
		error_descriptor m_error_code = {0, ""};

	private:
		int PollOpenForTx(std::string const& fifo_path, unsigned long start_time, unsigned long timeout_seconds);
		
		std::vector<std::string> m_current_messages = {};
};

// Server derived class.
class SatTerm_Server : public SatTerm_Component {
	public:
		SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages = true,
		               std::string const& terminal_emulator_paths = "./terminal_emulator_paths.txt", size_t stop_fifo_index = 0,
		               size_t sc_fifo_count = 1, size_t cs_fifo_count = 1, char end_char = 3, std::string const& stop_message = "q");
		~SatTerm_Server();
		
	private:
		std::string GetWorkingPath(void);
		bool CreateFifos(size_t sc_fifo_count, size_t cs_fifo_count);
		std::vector<std::string> __CreateFifos(size_t fifo_count, std::string const& fifo_identifier_prefix);
		pid_t StartClient(std::string const& path_to_terminal_emulator_paths);
		std::vector<std::string> LoadTerminalEmulatorPaths(std::string const& file_path);
		bool OpenFifos(unsigned long timeout_seconds);
		
		std::string m_path_to_client_binary = "";
};

// Client derived class.
class SatTerm_Client : public SatTerm_Component {
	public:
		SatTerm_Client(std::string const& identifier, int argc, char* argv[], bool display_messages = true);
		~SatTerm_Client();
		
	private:
		size_t GetArgStartIndex(int argc, char* argv[]);
		std::vector<std::string> ParseFifoPaths(size_t argv_start_index, size_t argv_count, char* argv[]);
		bool OpenFifos(unsigned long timeout_seconds);
};

class Fifo {
	public:
		Fifo() {};
		virtual ~Fifo() {};
		
		virtual bool Open(unsigned long timeout_seconds) = 0;
		virtual std::string GetMessage(bool capture_end_char, unsigned long timeout_seconds) = 0;
		virtual std::string SendMessage(std::string const& message, unsigned long timeout_seconds) = 0;
		virtual size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds) = 0;
		
		bool Create(void);
		
	protected:
		error_descriptor GetErrorCode(void);
		
		bool m_display_messages = false;
		error_descriptor m_error_code = {0, ""};
		
		std::string m_identifier = "";
		std::string m_working_path = "";;
		bool m_connected = false;
		int m_fifo_descriptor = 0;
		
};

class Out_Port : public Fifo {
	public:
		Out_Port(std::string const& identifier, bool display_messages);
		~Out_Port();
		
		bool Open(unsigned long timeout_seconds = 5) override;
		std::string GetMessage(bool capture_end_char = false, unsigned long timeout_seconds = 0) override;
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds = 5) override;
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds = 5) override;
		
		bool PollToOpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		
	private:
		
} ;

class In_Port : public Fifo {
	public:
		In_Port(std::string const& identifier, bool display_messages);
		~In_Port();
		
		bool Open(unsigned long timeout_seconds = 5) override;
		std::string GetMessage(bool capture_end_char = false, unsigned long timeout_seconds = 0) override;
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds = 5) override;
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds = 5) override;
	
	private:
		std::string m_current_message;
} ;
