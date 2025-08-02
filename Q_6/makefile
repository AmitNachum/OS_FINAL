CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Pull port number from network_interface.hpp
PORT_NUM := $(shell sed -n 's/#define PORT *"\(.*\)"/\1/p' network_interface.hpp)

TARGETS = server client

# Default target: build both server and client
all: $(TARGETS)

# Link targets with CXX (ensures libstdc++ is used)
server: server.o
	$(CXX) $(CXXFLAGS) -o $@ $^

client: client.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule for building object files
%.o: %.cpp network_interface.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all clean

# Clean up: kill any processes holding the PORT, then remove artifacts
clean:
	-@echo "Killing processes listening on TCP port $(PORT_NUM)..."
	-@lsof -ti tcp:$(PORT_NUM) | xargs -r kill -9
	 rm -f *.o $(TARGETS)
