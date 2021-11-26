#include <unistd.h>					// sleep().
#include <string>
#include <vector>

#include <iostream>

#include "satellite_terminal.h"

int main(int argc, char *argv[]) {
	
	size_t argv_start_index = 1;
	SatTerm_Client stc("test_client", '\n', argv_start_index, argv, true);

	if (stc.IsInitialised()) {
		bool running = true;
		while (running) {
			for (size_t i = 0; i < stc.GetRxFifoCount(); i ++) {
				std::string message = stc.GetMessage(false, i);
				if (message != "") {
					if ((message == "q") && (i == 0)) {
						running = false;
						break;
					} else {
						std::cout << message << std::endl;
					}
				}
				usleep(1000);
			}
		}
	} else {
		std::cout << "Client initialisation failed." << std::endl;
		sleep(5);
	}
	return 0;
}
