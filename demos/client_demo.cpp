#include <unistd.h>					// sleep().
#include <string>
#include <vector>
#include <ctime>
#include <iostream>

#include "satellite_terminal.h"

int main(int argc, char *argv[]) {
	
	size_t argv_start_index = 1;
	SatTerm_Client stc("test_client", '\n', argv_start_index, argv, true);

	if (stc.IsInitialised()) {
		
		std::string inbound_message = "";
		
		while ((stc.GetErrorCode().err_no == 0) && (inbound_message != "q")) {
			inbound_message = stc.GetMessage();
			if (inbound_message != "") {
				std::cout << inbound_message << std::endl;
				stc.SendMessage(inbound_message);
			}
			usleep(1000);
		}
	} else {
		std::cout << "Client initialisation failed." << std::endl;
		sleep(5);
	}
	return 0;
}
