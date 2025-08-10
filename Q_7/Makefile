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
.PHONY: all clean clean_coverage \
        valgrind_server valgrind_client helgrind_server helgrind_client callgrind_server callgrind_client \
        cover_server cover_client
all: server client

# Build the server executable
server: $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o $@

# Build the client executable
client: $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) -o $@

# ---------- Valgrind helpers ----------
valgrind_server:
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./server

valgrind_client:
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./client

helgrind_server:
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes ./server

helgrind_client:
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes ./client

callgrind_server:
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes ./server

callgrind_client:
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes ./client

# Swallow extra words after the first target so make doesn't treat them as targets
%:
	@:
# ---------- Code coverage (robust) ----------
# Requires in code:
#   #ifndef GCOV_MODE
#   #define GCOV_MODE 0
#   #endif
# and after you reply to the client in server.cpp:
#   #if GCOV_MODE
#   return 0; // exit after one request
#   #endif
# ---------- Coverage (Q_8) ----------
.PHONY: cover_server cover_client clean_coverage

# Clean coverage artifacts anywhere (recursively)
clean_coverage:
	@find . -type f \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) -delete
	@find ../Q_7 -type f \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) -delete
	@rm -f server_cov client_cov
	@echo "[COVER] cleaned"

# Build + run SERVER with coverage; expects your code to honor GCOV_MODE (exit after one request)
cover_server: clean_coverage
	@echo "[COVER] compile server objects WITH coverage (creates fresh .gcno)"
	# server.cpp compiles here
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c server.cpp -o server.o
	# Server_Socket.cpp compiles in its own directory so .gcno/.gcda live next to the source
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/Server_Socket.o

	@echo "[COVER] sanity: .gcno should exist"
	@ls -l server.gcno ../Q_7/Socket_class/Server_Socket.gcno

	@echo "[COVER] link server_cov"
	g++ --coverage server.o ../Q_7/Socket_class/Server_Socket.o -pthread -o server_cov

	@echo "[COVER] run server_cov (it should exit due to GCOV_MODE once one request is handled)"
	./server_cov || true

	@echo "[COVER] sanity: .gcda should exist"
	@ls -l server.gcda ../Q_7/Socket_class/Server_Socket.gcda

	@echo "[COVER] generate .gcov next to sources"
	gcov -b -c -o . server.cpp
	gcov -b -c -o ../Q_7/Socket_class ../Q_7/Socket_class/Server_Socket.cpp
	@echo "== GCOV (server) DONE =="

# Build + run CLIENT with coverage; if client needs input, provide it via coverage.in
cover_client: clean_coverage
	@echo "[COVER] compile client objects WITH coverage (creates fresh .gcno)"
	# client.cpp compiles here
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c client.cpp -o client.o
	# Client_Socket.cpp compiles in its own directory so .gcno/.gcda live next to the source
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c ../Q__7/Socket_class/Client_Socket.cpp -o ../Q_7/Socket_class/Client_Socket.o

	@echo "[COVER] sanity: .gcno should exist"
	@ls -l client.gcno ../Q_7/Socket_class/Client_Socket.gcno

	@echo "[COVER] link client_cov"
	g++ --coverage client.o ../Q_7/Socket_class/Client_Socket.o -pthread -o client_cov

	@echo "[COVER] run client_cov"
	@if [ -f coverage.in ]; then ./client_cov < coverage.in; else ./client_cov; fi || true

	@echo "[COVER] sanity: .gcda should exist"
	@ls -l client.gcda ../Q_7/Socket_class/Client_Socket.gcda

	@echo "[COVER] generate .gcov next to sources"
	gcov -b -c -o . client.cpp
	gcov -b -c -o ../Q_7/Socket_class ../Q_7/Socket_class/Client_Socket.cpp
	@echo "== GCOV (client) DONE =="
# ---------- Cleanup ----------


clean_coverage:
	@find . -type f \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) \
	    -not -path './.git/*' -delete
	@rm -rf .cov server_cov client_cov .srvpid
	@echo "Coverage artifacts removed."


# ---------- Cleanup ----------
clean: clean_coverage 
	rm -f server client *.o *.gcno *.gcda *.gcov callgrind.*.out *_cov 

clean_coverage:
	@for d in . $(shell find . -type d); do \
		rm -f $$d/*.o $$d/*.gcno $$d/*.gcda $$d/*.gcov $$d/*.o 2>/dev/null || true; \
	done
