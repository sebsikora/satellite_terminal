#include <unistd.h>					// sleep().
#include <string>
#include <vector>
#include <ctime>
#include <iostream>

#include "satellite_terminal.h"

int main(int argc, char *argv[]) {
	
	size_t argv_start_index = 1;
	SatTerm_Client stc("test_client", argv_start_index, argv);

	if (stc.IsConnected()) {
		while (stc.GetErrorCode().err_no == 0) {
			std::string inbound_message = stc.GetMessage();
			if (inbound_message != "") {
				std::cout << inbound_message << std::endl;
				if (inbound_message != stc.GetStopMessage()) {
					stc.SendMessage(inbound_message);
				} else {
					break;
				}
			}
			usleep(1000);
		}
		if (stc.GetErrorCode().err_no != 0) {
			std::cout << stc.GetErrorCode().err_no << "    " << stc.GetErrorCode().function << std::endl;
		}
		sleep(10);
	} else {
		if (stc.GetErrorCode().err_no != 0) {
			std::cout << stc.GetErrorCode().err_no << "    " << stc.GetErrorCode().function << std::endl;
		}
		sleep(10);
	}
	return 0;
}
