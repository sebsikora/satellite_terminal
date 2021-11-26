#include <unistd.h>					// sleep().

#include <iostream>
#include <string>

#include "satellite_terminal.h"

int main () {
	int sc_pipe_count = 4;
	int cs_pipe_count = 2;
	SatTerm_Server sts("test_server", "./client_demo", '\n', "q\n", true, 0, sc_pipe_count, cs_pipe_count);
	if (sts.IsInitialised()) {
		for (int i = 0; i < 10; i ++) {
			for (int j = 0; j < sc_pipe_count; j ++) {
				std::string message = "Message number " + std::to_string(i) + "_" + std::to_string(j);
				sts.SendMessage(message, j);
			}
		}
		sleep(10);
	} else {
		std::cout << "Server initialisation failed." << std::endl;
		sleep(10);
	}
	return 0;
}
