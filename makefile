CPPC=g++
CPPFLAGS=-Wall -g -O3
CPPLIBS=

CORE_INC=-I src/

CORE_SRC:=$(wildcard src/*.cpp)

server_demo: demos/server_demo.cpp $(CORE_SRC)
	$(CPPC) $(CPPLIBS) $(CPPFLAGS) $(CORE_INC) $(CORE_SRC) demos/$@.cpp -o $@

client_demo: demos/client_demo.cpp $(CORE_SRC)
	$(CPPC) $(CPPLIBS) $(CPPFLAGS) $(CORE_INC) $(CORE_SRC) demos/$@.cpp -o $@
