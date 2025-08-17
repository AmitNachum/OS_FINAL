# ---- Stage 9 Makefile (run from Q_9/) ----
# Builds ./server and ./client using Graph (../Q_1_to_4) and Stage 7 code (local)

# Sources (relative to Q_9/)
CLIENT_SRCS := client.cpp Socket_class/Client_Socket.cpp
SERVER_SRCS := server.cpp Socket_class/Server_Socket.cpp pipeline/Pipeline.cpp

# Compiler/flags
CXX       := g++
CXXSTD    := -std=c++17
CXXWARN   := -Wall -Wextra -Wpedantic
CXXOPT    := -O2
CXXMT     := -pthread
CXXFLAGS  := $(CXXSTD) $(CXXWARN) $(CXXOPT) $(CXXMT) -MMD -MP

# Includes (relative to Q_9/)
INCLUDES := -I. \
            -I../Q_1_to_4/Graph \
            -I./Socket_class \
            -I./Strategy \
            -I./Factory \
            -I./pipeline

LDFLAGS   := $(CXXMT)

# Outputs
OBJ_DIR     := build/obj
SERVER_EXE  := server
CLIENT_EXE  := client

# Objects
OBJS_CLIENT := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(CLIENT_SRCS)))
OBJS_SERVER := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SERVER_SRCS)))

# Default
.PHONY: all
all: $(SERVER_EXE) $(CLIENT_EXE)

# Link rules (file targets)
$(SERVER_EXE): $(OBJS_SERVER)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

$(CLIENT_EXE): $(OBJS_CLIENT)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS)

# Compile pattern
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Deps
-include $(OBJS_CLIENT:.o=.d)
-include $(OBJS_SERVER:.o=.d)

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

# ---------- Coverage (Stage 9) ----------
.PHONY: cover_build cover_run_server cover_run_client clean_coverage gcov_report

COVER_CXXFLAGS = -O0 -g --coverage -std=c++17 -fprofile-arcs -ftest-coverage -Wall -Wextra -Wpedantic -pthread \
                 -I. -Ipipeline -I./Socket_class -I./Strategy -I../Q_1_to_4/Graph -I./Factory
COVER_LDFLAGS  = --coverage -pthread
GCOV_BUILD_DIR := ./coverage_build

clean_coverage:
	@echo "[COVER] cleaned old coverage files"
	@rm -rf *.gcno *.gcda *.gcov pipeline/*.gcno pipeline/*.gcda pipeline/*.gcov Socket_class/*.gcno Socket_class/*.gcda Socket_class/*.gcov $(GCOV_BUILD_DIR)
	@echo "[COVER] cleaned"

# NEW & CORRECTED: Compile each source file individually
cover_build: clean_coverage
	@echo "[COVER] Compiling all sources for coverage"
	@mkdir -p $(GCOV_BUILD_DIR)
	g++ $(COVER_CXXFLAGS) -c server.cpp -o $(GCOV_BUILD_DIR)/server.o
	g++ $(COVER_CXXFLAGS) -c Socket_class/Server_Socket.cpp -o $(GCOV_BUILD_DIR)/Server_Socket.o
	g++ $(COVER_CXXFLAGS) -c pipeline/Pipeline.cpp -o $(GCOV_BUILD_DIR)/Pipeline.o
	g++ $(COVER_CXXFLAGS) -c client.cpp -o $(GCOV_BUILD_DIR)/client.o
	g++ $(COVER_CXXFLAGS) -c Socket_class/Client_Socket.cpp -o $(GCOV_BUILD_DIR)/Client_Socket.o
	
	@echo "[COVER] Linking executables for coverage"
	g++ $(COVER_LDFLAGS) $(GCOV_BUILD_DIR)/server.o $(GCOV_BUILD_DIR)/Server_Socket.o $(GCOV_BUILD_DIR)/Pipeline.o -o $(GCOV_BUILD_DIR)/server_cov
	g++ $(COVER_LDFLAGS) $(GCOV_BUILD_DIR)/client.o $(GCOV_BUILD_DIR)/Client_Socket.o -o $(GCOV_BUILD_DIR)/client_cov

cover_server:
	@echo "[COVER] Running server. Use make cover_run_client to test. Press CTRL+C to stop."
	@./$(GCOV_BUILD_DIR)/server_cov

cover_client:
	@echo "[COVER] Running client. You must run a server in another terminal."
	@if [ -f coverage.in ]; then ./$(GCOV_BUILD_DIR)/client_cov < coverage.in; else ./$(GCOV_BUILD_DIR)/client_cov; fi

gcov_report:
	@echo "[COVER] Collecting coverage reports"
	@gcov -b -c -r --object-directory=$(GCOV_BUILD_DIR) $(SERVER_SRCS)
	@gcov -b -c -r --object-directory=$(GCOV_BUILD_DIR) $(CLIENT_SRCS)
	@echo "== GCOV Reports Printed =="

# Phony helpers (avoid circular names)
.PHONY: build-server build-client clean print
build-server: $(SERVER_EXE)
build-client: $(CLIENT_EXE)

print:
	@echo "SERVER_SRCS:"; echo $(SERVER_SRCS) | tr ' ' '\n'
	@echo ""; echo "CLIENT_SRCS:"; echo $(CLIENT_SRCS) | tr ' ' '\n'
	@echo ""; echo "INCLUDES:"; echo $(INCLUDES)

clean: clean_coverage
	-lsof -t -i:5555 | xargs kill || true
	@echo "Cleaning Stage 9 build artifacts..."
	@rm -rf $(OBJ_DIR) $(SERVER_EXE) $(CLIENT_EXE)
	@rm -f *.o client_cov server_cov
	@rm -f pipeline/*.o Socket_class/*.o