# ---- Stage 6/7/8 Makefile with Build + Coverage + Tools ----

CXX := g++
CXXSTD := -std=c++20
CXXWARN := -Wall -Wextra -Wpedantic
CXXMT := -pthread
CXXFLAGS := $(CXXSTD) $(CXXWARN) $(CXXMT)

INCLUDES := -I. -I../Q_1_to_4/Graph

# Sources
SERVER_SRCS := server.cpp
CLIENT_SRCS := client.cpp

SERVER_EXE := server
CLIENT_EXE := client

GCOV_BUILD_DIR := ./coverage_build

VALGRIND_FLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --quiet

# ---------- Normal Build ----------

.PHONY: all
all: $(SERVER_EXE) $(CLIENT_EXE)

$(SERVER_EXE): $(SERVER_SRCS)
	@echo "Compiling server"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(CLIENT_EXE): $(CLIENT_SRCS)
	@echo "Compiling client"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# ---------- Coverage ----------

.PHONY: cover_build cover_server cover_client clean_coverage gcov_report

COVER_CXXFLAGS := -O0 -g --coverage -fprofile-arcs -ftest-coverage $(CXXSTD) $(CXXWARN) $(CXXMT) $(INCLUDES)
COVER_LDFLAGS := --coverage $(CXXMT)

clean_coverage:
	@echo "[COVER] Cleaning old coverage files"
	@rm -rf *.gcno *.gcda *.gcov $(GCOV_BUILD_DIR)
	@mkdir -p $(GCOV_BUILD_DIR)
	@echo "[COVER] Done cleaning"

cover_build: clean_coverage
	@echo "[COVER] Compiling server and client for coverage"
	$(CXX) $(COVER_CXXFLAGS) -c $(SERVER_SRCS) -o $(GCOV_BUILD_DIR)/server.o
	$(CXX) $(COVER_CXXFLAGS) -c $(CLIENT_SRCS) -o $(GCOV_BUILD_DIR)/client.o
	
	$(CXX) $(COVER_LDFLAGS) $(GCOV_BUILD_DIR)/server.o -o $(GCOV_BUILD_DIR)/server_cov
	$(CXX) $(COVER_LDFLAGS) $(GCOV_BUILD_DIR)/client.o -o $(GCOV_BUILD_DIR)/client_cov

cover_server:
	@echo "[COVER] Running server coverage build"
	@./$(GCOV_BUILD_DIR)/server_cov

cover_client:
	@echo "[COVER] Running client coverage build"
	@./$(GCOV_BUILD_DIR)/client_cov $(ARGS)

gcov_report:
	@echo "[COVER] Collecting coverage reports"
	@gcov -b -c -r --object-directory=$(GCOV_BUILD_DIR) $(SERVER_SRCS)
	@gcov -b -c -r --object-directory=$(GCOV_BUILD_DIR) $(CLIENT_SRCS)
	@echo "== GCOV Reports Printed =="

# ---------- Valgrind / Helgrind / Callgrind ----------

.PHONY: valgrind_server valgrind_client helgrind_server helgrind_client callgrind_server callgrind_client

valgrind_server: $(SERVER_EXE)
	@valgrind $(VALGRIND_FLAGS) ./$(SERVER_EXE)

valgrind_client: $(CLIENT_EXE)
	@echo "Running client with arguments: $(ARGS)"
	@valgrind $(VALGRIND_FLAGS) ./$(CLIENT_EXE) $(ARGS)

helgrind_server: $(SERVER_EXE)
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes ./$(SERVER_EXE)

helgrind_client: $(CLIENT_EXE)
	@valgrind --tool=helgrind --history-level=full --track-lockorders=yes --fair-sched=yes ./$(CLIENT_EXE) $(ARGS)

callgrind_server: $(SERVER_EXE)
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes ./$(SERVER_EXE)

callgrind_client: $(CLIENT_EXE)
	@valgrind --tool=callgrind --callgrind-out-file=callgrind.$@.out --dump-instr=yes --collect-jumps=yes ./$(CLIENT_EXE) $(ARGS)

# ---------- Clean ----------

.PHONY: clean
clean: clean_coverage
	@echo "Cleaning build artifacts..."
	@rm -f $(SERVER_EXE) $(CLIENT_EXE) *.o *.gcno *.gcda *.gcov server_cov client_cov
