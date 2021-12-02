#include <string>                    // std::string.
#include <vector>                    // std::vector.
#include <map>
#include <memory>

#include "satterm_port.h"

class SatTerm_Agent {
	public:
		SatTerm_Agent() {}
		virtual ~SatTerm_Agent() {}
		
		std::string GetMessage(bool capture_end_char = false, unsigned long timeout_seconds = 0);
		std::string GetMessage(std::string const& port_identifier, bool capture_end_char = false, unsigned long timeout_seconds = 0);
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds = 0);
		std::string SendMessage(std::string const& message, std::string const& port_identifier, unsigned long timeout_seconds = 5);
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds = 5);
		size_t SendBytes(const char* bytes, size_t byte_count, std::string const& port_identifier, unsigned long timeout_seconds = 5);
		
		error_descriptor GetErrorCode(void);
		std::string GetStopPortIdentifier(void);
		std::string GetStopMessage(void);
		bool IsConnected(void);
		void SetConnectedFlag(bool is_connected);
		
	protected:
		bool CreatePorts(bool is_server, bool is_in_port, std::string const& working_path, std::vector<std::string> port_identifiers,
		                 bool display_messages, char end_char, std::map<std::string, std::unique_ptr<Port>>& ports);
		bool OpenPorts(std::map<std::string, std::unique_ptr<Port>>& ports, unsigned long timeout_seconds = 5);
		
		error_descriptor m_error_code = {0, ""};
		bool m_display_messages = false;
		std::map<std::string, std::unique_ptr<Port>> m_in_ports = {};
		std::map<std::string, std::unique_ptr<Port>> m_out_ports = {};
		default_port m_default_port = {"", ""};
		std::string m_stop_port_identifier = "";
		std::string m_working_path = "";
		std::string m_identifier = "";
		std::string m_stop_message = "";
		char m_end_char = 0;
		bool m_connected = false;
};

class SatTerm_Server : public SatTerm_Agent {
	public:
		SatTerm_Server(std::string const& identifier, std::string const& path_to_client_binary, bool display_messages = true,
                       std::string const& path_to_terminal_emulator_paths = "./terminal_emulator_paths.txt",
                       std::vector<std::string> out_port_identifiers = {"server_tx"}, std::vector<std::string> in_port_identifiers = {"server_rx"},
                       char end_char = 3, std::string const& stop_port_identifier = "", std::string const& stop_message = "q");
		~SatTerm_Server();
		
	private:
		std::string GetWorkingPath(void);
		std::vector<std::string> LoadTerminalEmulatorPaths(std::string const& file_path);
		pid_t StartClient(std::string const& path_to_terminal_emulator_paths, std::string const& path_to_client_binary, std::string const& working_path,
                          char end_char, std::string const& stop_message, std::vector<std::string> in_port_identifiers,
                          std::vector<std::string> out_port_identifiers);
		
};

class SatTerm_Client : public SatTerm_Agent {
	public:
		SatTerm_Client(std::string const& identifier, int argc, char* argv[], bool display_messages = true);
		~SatTerm_Client();

	private:
		size_t GetArgStartIndex(std::string const& arg_start_delimiter, int argc, char* argv[]);
		std::vector<std::string> ParseFifoPaths(size_t argv_start_index, size_t argv_count, char* argv[]);
};
