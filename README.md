# satellite_terminal
<br />

© 2021 Dr Sebastien Sikora.

[seb.nf.sikora@protonmail.com](mailto:seb.nf.sikora@protonmail.com)
<br />
<br />

A simple C++ library for POSIX compatible operating systems that enables your project to easily spawn and communicate bidirectionally with client processes in separate terminal emulator instances.
<br />

Updated 29/11/2021.
<br />
<br />

## What is it?

When developing a C++ project on for POSIX compatible operting systems (eg GNU/Linux), you may wish to run one or more additional child processes each with access to their own terminal emulator instance, and the ability to communicate bi-directionally with the parent process.

For instance, in the simplest case if the parent process occupies it's terminal emulator instance with it's own output, you may wish to spawn a child process with it's own terminal emulator instance to display messages from the parent process and receive user input to send back.

This can be implemented within a C or C++ project on compatible operting systems using the functionality defined in the [unistd.h](https://en.wikipedia.org/wiki/Unistd.h), [sys/stat.h](https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/sys/stat.h) & [fcntl.h](https://pubs.opengroup.org/onlinepubs/007904875/basedefs/fcntl.h.html) headers to spawn a child process, and create [named pipes](https://en.wikipedia.org/wiki/Named_pipe) that provide a communication channel to-which and from-which the parent and child processes can write and read.

satellite_terminal is a simple library that makes it easy to incorporate this functionality within your own C++ projects by fully automating this process.
<br />
<br />

## How to use it?

Using satellite_terminal in a C++ project is very easy. Let's demonstrate this via a trivial example.
<br />
<br />

### Parent process

The parent process spawns the child process by instantiating a SatTerm_Server. The server constructor is passed an identifier string and the path to the child process binary as arguments.

By default two named pipes will be created to form a tx-rx pair, but an arbitrary of tx and rx named pipes can be created if desired.
<br />

```cpp
// parent.cpp
#include "satellite_terminal.h"
...

SatTerm_Server sts("test_server", "./child_binary");

// Path to child binary above can incorporate desired command-line arguments
// eg: "./client_demo --full-screen=true"

if (sts.IsConnected()) {
	// We are good to go!
	...
}
...
```
<br />

The server constructor will create the named pipe temporary files in the local directory and then spawn a terminal emulator (from the list in terminal_emulator_paths.txt) within-which it will directly execute the child binary via the '-e' option. The paths to the named pipes are passed to the child binary as command-line options by appending them to the child binary path string.

The server constructor will return once the communication channel is established with the child process, an error occurs or a timeout is reached. When it returns, if the server's `IsConnected()` member function returns `true`, the child process started correctly and the bi-directional communication channel was established without error.
<br />
<br />

### Child process

The paths to the named pipes are passed to the child process via it's command-line arguments, therefore argc and argv must be passed to the SatTerm_Client constructor.

The pipe path data is appended directly onto the child binary path string passed to the server constructor following an automatically applied delimiter ("client_args"), so you can use any command-line arguments required by the child process as normal and client constructor will automatically parse the remaining arguments.
<br />

```cpp
// child.cpp
#include "satellite_terminal.h"

int main(int argc, char *argv[]) {
	
	// -- Your argument parser goes here ---
	//      -- Don't modify argc/v! --
	
	SatTerm_Client stc("test_client", argc, argv);
	
	if (stc.IsConnected()) {
		// We are good to go!
		...
	}
...
```
<br />

The client constructor will return once the communication channel is established with the parent process, an error occurs or a timeout is reached. When it returns, if the client's `IsConnected()` member function returns `true`,the bi-directional communication channel was established without error.
<br />
<br />

### Sending and receiving

Blah...
<br />

```cpp

size_t sent_bytes = stc.SendMessage("Outbound message");

std::string inbound_message = stc.GetMessage();

```
<br />

Blah [blah]() `blah.cpp`.

Blah.
<br />
<br />

## Complete basic example

Blah...
<br />

```cpp
// server_demo.cpp

#include <unistd.h>                 // sleep(), usleep().
#include <ctime>                    // time().
#include <iostream>                 // std::cout, std::endl.
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
			std::cout << sts.GetErrorCode().err_no << "    " << sts.GetErrorCode().function << std::endl;
		}
		
	} else {
		if (sts.GetErrorCode().err_no != 0) {
			std::cout << sts.GetErrorCode().err_no << "    " << sts.GetErrorCode().function << std::endl;
		}
		sleep(5);
	}
	return 0;
}
```
<br />

Blah...
<br />

```cpp
// client_demo.cpp

#include <unistd.h>                 // sleep(), usleep().
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
```
<br />

Compile and run:
<br />

```
user@home:~/Documents/cpp_projects/satellite_terminal$ ./server_demo 
Fifo working path is /home/user/Documents/cpp_projects/satellite_terminal
Client process started.
Server test_server opened fifo test_server_fifo_cs_0 for reading on descriptor 3
Server test_server opened fifo test_server_fifo_sc_0 for writing on descriptor 4
Server test_server initialised successfully.
Message "Message number 0 from server." returned by client.
Message "Message number 1 from server." returned by client.
Message "Message number 2 from server." returned by client.
Message "Message number 3 from server." returned by client.
Message "Message number 4 from server." returned by client.
Message "Message number 5 from server." returned by client.
Message "Message number 6 from server." returned by client.
Message "Message number 7 from server." returned by client.
Message "Message number 8 from server." returned by client.
Message "Message number 9 from server." returned by client.
Waiting for client process to terminate...
EOF on read() to fifo index 0 suggests counterpart terminated.
user@home:~/Documents/cpp_projects/satellite_terminal$
```
<br />

```
Fifo working path is /home/sebsikora/Documents/cpp_projects/satellite_terminal/
Client test_client opened fifo test_server_fifo_cs_0 for writing on descriptor 3
Client test_client opened fifo test_server_fifo_sc_0 for reading on descriptor 4
Client test_client initialised successfully.
Message number 0 from server.
Message number 1 from server.
Message number 2 from server.
Message number 3 from server.
Message number 4 from server.
Message number 5 from server.
Message number 6 from server.
Message number 7 from server.
Message number 8 from server.
Message number 9 from server.
q
```
<br />

Blah...
<br />
<br />

## License:
<br />

![Mit License Logo](./220px-MIT_logo.png)
<br />
<br />

satellite_terminal is distributed under the terms of the MIT license.
Learn about the MIT license [here](https://choosealicense.com/licenses/mit/)

