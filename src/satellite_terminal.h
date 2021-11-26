/*
	
	satellite_terminal - Easily spawn and communicate by-directionally with a client process in a separate terminal emulator instance.
	
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

class SatTerm_Component {
	public:
		SatTerm_Component (std::string const& identifier, bool display_messages);
		virtual ~SatTerm_Component () {}
		std::string GetMessage(bool capture_end_char, int rx_fifo_index = 0);
		int SendMessage(std::string message, int tx_fifo_index = 0);
		int SendBytes(const char* bytes, int byte_count, int tx_fifo_index = 0);
		bool IsInitialised(void);
		int GetTxFifoCount(void);
		int GetRxFifoCount(void);
		int GetErrorCode(void);

	protected:
		bool OpenRxFifos(void);
		bool OpenTxFifos(void);
		
		bool m_display_messages = false;
		std::string m_component_type = "";
		std::string m_identifier = "";
		char m_end_char = 0;
		std::vector<std::string> m_current_messages = {};
		std::vector<int> m_tx_fifo_descriptors = {};
		std::vector<int> m_rx_fifo_descriptors = {};
		std::vector<std::string> m_rx_fifo_paths = {};
		std::vector<std::string> m_tx_fifo_paths = {};
		bool m_initialised_successfully = false;
		int m_error_code = 0;
};

class SatTerm_Server : public SatTerm_Component {
	public:
		SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, char end_char, std::string const& stop_signal,
						bool display_messages = false, int stop_fifo_index = 0, int sc_fifo_count = 1, int cs_fifo_count = 1);
		~SatTerm_Server();
		int Stop(void);
		
	private:
		pid_t StartClient(void);
		bool CreateFifos(int sc_fifo_count, int cs_fifo_count);
		bool OpenFifos(void);
		int Finish(void);
		
		pid_t m_client_pid = 0;
		std::string m_path_to_client_binary = "";
		int m_stop_fifo_index = 0;
		std::string m_stop_signal = "";
};

class SatTerm_Client : public SatTerm_Component {
	public:
		SatTerm_Client(std::string const& identifier, char end_char, std::vector<std::string> rx_fifo_paths, std::vector<std::string> tx_fifo_paths, bool display_messages = false);
		SatTerm_Client(std::string const& identifier, char end_char, int argv_start_index, char* argv[], bool display_messages = false);
		
	private:
		bool OpenFifos(void);
};
