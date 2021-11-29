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
		size_t SendMessage(std::string const& message, size_t tx_fifo_index = 0, unsigned long timeout_seconds = 10);
		size_t SendBytes(const char* bytes, size_t byte_count, size_t tx_fifo_index = 0, unsigned long timeout_seconds = 10);
		bool IsConnected(void);
		size_t GetTxFifoCount(void);
		size_t GetRxFifoCount(void);
		error_descriptor GetErrorCode(void);
		std::string GetStopMessage(void);
		
	protected:
		bool OpenRxFifos(unsigned long timeout_seconds);
		bool OpenTxFifos(unsigned long timeout_seconds);
		
		bool m_display_messages = false;
		std::string m_component_type = "";
		std::string m_identifier = "";
		char m_end_char = 0;
		std::string m_stop_message = "";
		std::vector<int> m_tx_fifo_descriptors = {};
		std::vector<int> m_rx_fifo_descriptors = {};
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
		SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages = true, size_t stop_fifo_index = 0,
		               size_t sc_fifo_count = 1, size_t cs_fifo_count = 1, char end_char = '\n', std::string const& stop_message = "q");
		~SatTerm_Server();
		
	private:
		std::vector<std::string> LoadTerminalEmulatorPaths(std::string const& file_path);
		pid_t StartClient(std::string const& path_to_terminal_emulator_paths);
		bool CreateFifos(size_t sc_fifo_count, size_t cs_fifo_count);
		bool OpenFifos(unsigned long timeout_seconds);
		
		std::string m_path_to_client_binary = "";
		int m_stop_fifo_index = 0;
};

// Client derived class.
class SatTerm_Client : public SatTerm_Component {
	public:
		SatTerm_Client(std::string const& identifier, std::vector<std::string> rx_fifo_paths, std::vector<std::string> tx_fifo_paths, bool display_messages = true, char end_char = '\n', std::string const& stop_message = "q");
		SatTerm_Client(std::string const& identifier, size_t argv_start_index, char* argv[], bool display_messages = true, char end_char = '\n', std::string const& stop_message = "q");
		~SatTerm_Client();
		
	private:
		std::vector<std::string> ParseVarargs(size_t argv_start_index, size_t argv_count, char* argv[]);
		void Configure(void);
		bool OpenFifos(unsigned long timeout_seconds);
};
