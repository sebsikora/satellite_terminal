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

What is it?
-------------------------
When developing a C++ project on for POSIX compatible operting systems (eg GNU/Linux), you may wish to run one or more additional child processes each with access to their own terminal emulator instance, and the ability to communicate bi-directionally with the parent process.

For instance, in the simplest case if the parent process occupies it's terminal emulator instance with it's own output, you may wish to spawn a child process with it's own terminal emulator instance to display messages from the parent process and receive user input to send back.

This can be implemented within a C or C++ project on compatible operting systems using the functionality defined in the [unistd.h](https://en.wikipedia.org/wiki/Unistd.h), [sys/stat.h](https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/sys/stat.h) & [fcntl.h](https://pubs.opengroup.org/onlinepubs/007904875/basedefs/fcntl.h.html) headers to spawn a child process, and create [named pipes](https://en.wikipedia.org/wiki/Named_pipe) that provide a communication channel available to the parent and child processes.

satellite_terminal is a simple library that makes it easy to incorporate this functionality within your own C++ projects by fully automating this process.
<br />

How to use it?
-------------------------

<br />

```cpp
// blah...
```
<br />
<br />

Blah [blah]() `blah.cpp`.

Blah.
<br />

License:
-------------------------
![Mit License Logo](./220px-MIT_logo.png)
<br/>
<br/>

satellite_terminal is distributed under the terms of the MIT license.
Learn about the MIT license [here](https://choosealicense.com/licenses/mit/)

