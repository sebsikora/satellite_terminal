#include <string>                    // std::string.
#include <vector>                    // std::vector.

struct default_port {
	std::string tx;
	std::string rx;
	default_port& operator=(default_port const& rhs) {
		tx = rhs.tx;
		rx = rhs.rx;
		return *this;
	}
};

struct error_descriptor {
	int err_no;
	std::string detail;
	error_descriptor& operator=(error_descriptor const& rhs) {
		err_no = rhs.err_no;
		detail = rhs.detail;
		return *this;
	}
	bool operator==(error_descriptor const& rhs) const {
		return ((this->err_no == rhs.err_no) && (this->detail == rhs.detail));
	}
	bool operator!=(error_descriptor const& rhs) const {
		return ((this->err_no != rhs.err_no) || (this->detail != rhs.detail));
	}
};

class Port {
	public:
		Port(bool is_server, bool is_in_port, std::string const& working_path, std::string const& identifier, bool display_messages, char end_char);
		~Port();
		
		void Create(void);
		void Open(unsigned long timeout_seconds);
		void Close(void);
		
		std::string GetMessage(bool capture_end_char, unsigned long timeout_seconds);
		std::string SendMessage(std::string const& message, unsigned long timeout_seconds);
		size_t SendBytes(const char* bytes, size_t byte_count, unsigned long timeout_seconds);
		
		error_descriptor GetErrorCode(void);
		bool IsCreated(void);
		bool IsOpened(void);
		bool IsInPort(void);
	
	private:
		bool OpenRxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		bool OpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		int PollToOpenTxFifo(std::string const& fifo_path, unsigned long timeout_seconds);
		
		error_descriptor m_error_code = {0, ""};
		bool m_display_messages = false;
		bool m_is_server = false;
		bool m_is_in_port = false;
		bool m_created = false;
		bool m_opened = false;
		
		std::string m_identifier = "";
		std::string m_working_path = "";
		int m_fifo_descriptor = 0;
		char m_end_char = 0;
		std::string m_current_message = "";
};
