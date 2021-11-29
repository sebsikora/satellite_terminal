#include <unistd.h>					// sleep(), usleep().
#include <ctime>                    // time().
#include <iostream>                 // std::cout, std::endl.
#include <string>                   // std::string.

#include "satellite_terminal.h"

int main(int argc, char *argv[]) {
	
	SatTerm_Client stc("test_client", argc, argv);

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
		sleep(5);
	} else {
		if (stc.GetErrorCode().err_no != 0) {
			std::cout << stc.GetErrorCode().err_no << "    " << stc.GetErrorCode().function << std::endl;
		}
		sleep(5);
	}
	return 0;
}
