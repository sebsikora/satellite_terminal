#include <unistd.h>                 // sleep(), usleep().
#include <ctime>                    // time().
#include <iostream>                 // std::cout, std::cerr, std::endl.
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
		std::cerr << "On termination error code = " << stc.GetErrorCode().err_no << "    Error detail = " << stc.GetErrorCode().err_detail << std::endl;
		sleep(5);        // Delay to read the message before terminal emulator window closes.
	} else {
		std::cerr << "On termination error code = " << stc.GetErrorCode().err_no << "    Error detail = " << stc.GetErrorCode().err_detail << std::endl;
		sleep(5);        // Delay to read the message before terminal emulator window closes.
	}
	return 0;
}
