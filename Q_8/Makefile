CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread \
  -I. \
  -I../Q_1_to_4/Graph \
  -I../Q_6 \
  -I../Q_7 \
  -I../Q_7/Factory \
  -I../Q_7/Socket_class \
  -I../Q_7/Strategy

LDFLAGS := -pthread

# Binaries
all: server client

server: server.cpp ../Q_7/Socket_class/Server_Socket.cpp run_all.hpp
	$(CXX) $(CXXFLAGS) server.cpp ../Q_7/Socket_class/Server_Socket.cpp -o $@ $(LDFLAGS)

client: client.cpp ../Q_7/Socket_class/Client_Socket.cpp
	$(CXX) $(CXXFLAGS) client.cpp ../Q_7/Socket_class/Client_Socket.cpp -o $@ $(LDFLAGS)

clean:
	rm -f server client
