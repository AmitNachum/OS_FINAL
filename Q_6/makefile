CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../Q_1_to_4/Graph -pthread -O0 --coverage

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


.PHONY: valgrind_server valgrind_client helgrind_server helgrind_client callgrind_server callgrind_client

# ---------- Memcheck ----------
valgrind_server:
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./server $(filter-out $@,$(MAKECMDGOALS))

valgrind_client:
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./client $(filter-out $@,$(MAKECMDGOALS))

# ---------- Helgrind ----------
helgrind_server:
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes \
		./server $(filter-out $@,$(MAKECMDGOALS))

helgrind_client:
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes \
		./client $(filter-out $@,$(MAKECMDGOALS))

# ---------- Callgrind ----------
callgrind_server:
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes \
		./server $(filter-out $@,$(MAKECMDGOALS))

callgrind_client:
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes \
		./client $(filter-out $@,$(MAKECMDGOALS))

# Swallow extra words after the first target so make doesn't treat them as targets
%:
	@:
# --- Build the preload flusher once ---
gcovflush.so: gcov_flush.c
	$(CC) -fPIC -shared -o $@ $<

.PHONY: cover_client cover_server clean_coverage

# Client coverage run (supports: make cover_client random 4 0.5)
cover_client: client gcovflush.so
	@LD_PRELOAD=./gcovflush.so ./client $(filter-out $@,$(MAKECMDGOALS))
	@echo "== GCOV (client) =="
	@gcov -b -c -o . $(shell find . -maxdepth 2 -name '*.cpp') | grep -E 'File|Lines executed|Branch' || true

# Server coverage run (supports: make cover_server manual 8 0.2)
cover_server: CXXFLAGS += -DGCOV_MODE=1
cover_server: server gcovflush.so
	@LD_PRELOAD=./gcovflush.so ./server $(filter-out $@,$(MAKECMDGOALS)) || true
	@echo "== GCOV (server) =="
	@gcov -b -c -o . $(shell find . -maxdepth 2 -name '*.cpp') | grep -E 'File|Lines executed|Branch' || true

# Swallow extra words so make doesn't treat them as targets
%:
	@:

# Cleanup
clean_coverage:
	@rm -f *.gcda *.gcno *.gcov coverage.info


.PHONY: all clean

clean:
	-@echo "Killing processes listening on TCP port $(PORT_NUM)..."
	-@lsof -ti tcp:$(PORT_NUM) | xargs -r kill -9
	 rm -f *.o $(TARGETS) *.gcno *.gcda *.gcov gcov gcovflush.so
