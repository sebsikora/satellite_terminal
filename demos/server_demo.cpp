#include <unistd.h>					// sleep(), usleep().
#include <ctime>                    // time().
#include <iostream>                 // std::cout, std::cerr, std::endl.
#include <string>                   // std::string.

#include "satellite_terminal.h"

int main (void) {
	
	SatTerm_Server sts("test_server", "./client_demo");
	
	if (sts.IsConnected()) {
		size_t message_count = 10;
		for (size_t i = 0; i < message_count; i ++) {
			std::string outbound_message = "Message number " + std::to_string(i) + " from server.";
			sts.SendMessage(outbound_message);
		}

		unsigned long timeout_seconds = 5;
		unsigned long start_time = time(0);
		
		while ((sts.GetErrorCode().err_no == 0) && ((time(0) - start_time) < timeout_seconds)) {
			std::string inbound_message = sts.GetMessage();
			if (inbound_message != "") {
				std::cout << "Message \"" << inbound_message << "\" returned by client." << std::endl;
			}
			usleep(1000);
		}
		if (sts.GetErrorCode().err_no != 0) {
			std::cerr << sts.GetErrorCode().err_no << "    " << sts.GetErrorCode().function << std::endl;
		}
		
	} else {
		if (sts.GetErrorCode().err_no != 0) {
			std::cerr << sts.GetErrorCode().err_no << "    " << sts.GetErrorCode().function << std::endl;
		}
		sleep(5);
	}
	return 0;
}
