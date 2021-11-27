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

#include <string>
#include <vector>

struct error_descriptor {
	int err_no;
	std::string func_call;
	error_descriptor& operator=(error_descriptor const& rhs) {
		err_no = rhs.err_no;
		func_call = rhs.func_call;
		return *this;
	}
};

// Component base class. Functionality common to both server and client classes.
class SatTerm_Component {
	public:
		SatTerm_Component () {}				// Abstract base class constructor.
		virtual ~SatTerm_Component () {}	// virtual destructor for abstract base class.
		std::string GetMessage(unsigned long timeout_seconds = 0, bool capture_end_char = false, size_t rx_fifo_index = 0);
		int SendMessage(std::string const& message, size_t tx_fifo_index = 0);
		int SendBytes(const char* bytes, size_t byte_count, size_t tx_fifo_index = 0);
		bool IsInitialised(void);
		size_t GetTxFifoCount(void);
		size_t GetRxFifoCount(void);
		error_descriptor GetErrorCode(void);

	protected:
		bool OpenRxFifos(void);
		bool OpenTxFifos(void);
		
		bool m_display_messages = false;
		std::string m_component_type = "";
		std::string m_identifier = "";
		char m_end_char = 0;
		std::vector<int> m_tx_fifo_descriptors = {};
		std::vector<int> m_rx_fifo_descriptors = {};
		std::vector<std::string> m_rx_fifo_paths = {};
		std::vector<std::string> m_tx_fifo_paths = {};
		bool m_initialised_successfully = false;
		error_descriptor m_error_code = {0, ""};

	private:
		int PollToOpenFifoForRx(std::string const& fifo_path, unsigned long start_time, unsigned long timeout_seconds);
		bool PollToReceiveInitMessage(int fifo_descriptor, std::string const& fifo_path, unsigned long start_time, unsigned long timeout_seconds);

		std::vector<std::string> m_current_messages = {};
};

// Server derived class.
class SatTerm_Server : public SatTerm_Component {
	public:
		SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, char end_char, std::string const& stop_signal,
						bool display_messages = false, size_t stop_fifo_index = 0, size_t sc_fifo_count = 1, size_t cs_fifo_count = 1);
		~SatTerm_Server();
		int Stop(void);
		
	private:
		pid_t StartClient(void);
		bool CreateFifos(size_t sc_fifo_count, size_t cs_fifo_count);
		bool OpenFifos(void);
		int Finish(void);
		
		pid_t m_client_pid = 0;
		std::string m_path_to_client_binary = "";
		int m_stop_fifo_index = 0;
		std::string m_stop_signal = "";
};

// Client derived class.
class SatTerm_Client : public SatTerm_Component {
	public:
		SatTerm_Client(std::string const& identifier, char end_char, std::vector<std::string> rx_fifo_paths, std::vector<std::string> tx_fifo_paths, bool display_messages = false);
		SatTerm_Client(std::string const& identifier, char end_char, size_t argv_start_index, char* argv[], bool display_messages = false);
		
	private:
		void ParseVarargs(size_t argv_start_index, char* argv[], std::vector<std::string> &tx_fifo_paths_container, std::vector<std::string> &rx_fifo_paths_container);
		void Configure(void);
		bool OpenFifos(void);
};
