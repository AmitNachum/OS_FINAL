# Makefile for compiling server and client C++ programs

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
    -I. \
    -IQ_1_to_4/Graph \
    -IQ_6 \
    -IQ_7/Factory \
    -IQ_7/Socket_class \
    -IQ_7/Strategy

# Source files for each target
SERVER_SRCS := server.cpp Socket_class/Server_Socket.cpp
CLIENT_SRCS := client.cpp Socket_class/Client_Socket.cpp

# Default target builds both programs
.PHONY: all clean
all: server client

# Rule to build the server executable
server: $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o $@

# Rule to build the client executable
client: $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) -o $@

# Clean up generated binaries
clean:
	rm -f server client