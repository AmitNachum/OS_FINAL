CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../Q_1_to_4/Graph

# Pull port number from network_interface.hpp
PORT_NUM := $(shell sed -n 's/#define PORT *"\(.*\)"/\1/p' network_interface.hpp)

TARGETS = server client

# Default target: build both server and client
all: $(TARGETS)

server: server.o
	$(CXX) $(CXXFLAGS) -o $@ $^

client: client.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Pattern rule for building object files
%.o: %.cpp network_interface.hpp ../Q_1_to_4/Graph/Graph.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all clean

clean:
	-@echo "Killing processes listening on TCP port $(PORT_NUM)..."
	-@lsof -ti tcp:$(PORT_NUM) | xargs -r kill -9
	 rm -f *.o $(TARGETS)
