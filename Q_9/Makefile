# ---- Stage 9 Makefile (run from Q_9/) ----
# Builds ./server and ./client using Graph (../Q_1_to_4) and Stage 7 code (../Q_7)

# Sources (relative to Q_9/)
CLIENT_SRCS := client.cpp ../Q_7/Socket_class/Client_Socket.cpp
SERVER_SRCS := server.cpp ../Q_7/Socket_class/Server_Socket.cpp pipeline/Pipeline.cpp

# Compiler/flags
CXX      := g++
CXXSTD   := -std=c++17
CXXWARN  := -Wall -Wextra -Wpedantic
CXXOPT   := -O2
CXXMT    := -pthread
CXXFLAGS := $(CXXSTD) $(CXXWARN) $(CXXOPT) $(CXXMT) -MMD -MP

# Includes (relative to Q_9/)
INCLUDES := -I. \
            -I../Q_1_to_4/Graph \
            -I../Q_7 -I../Q_7/Strategy -I../Q_7/Factory -I../Q_7/Socket_class \
            -I./pipeline

LDFLAGS  := $(CXXMT)

# Outputs
OBJ_DIR     := build/obj
SERVER_EXE  := server
CLIENT_EXE  := client

# Objects
OBJS_CLIENT := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SRCS))
OBJS_SERVER := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SERVER_SRCS))

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
.PHONY: cover_server cover_client clean_coverage

COVER_CXXFLAGS = -O0 -g --coverage -std=c++17 -fprofile-arcs -ftest-coverage -Wall -Wextra -Wpedantic -pthread \
                 -I. -Ipipeline -I../Q_1_to_4/Graph -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy
COVER_LDFLAGS  = --coverage -pthread
# Disable GCOV_PREFIX to write .gcda next to .gcno
GCOV_ENV =

clean_coverage:
	@rm -f *.gcno *.gcda *.gcov
	@rm -f ../Q_7/Socket_class/*.gcno ../Q_7/Socket_class/*.gcda ../Q_7/Socket_class/*.gcov
	@rm -f pipeline/*.gcno pipeline/*.gcda pipeline/*.gcov
	@rm -rf .cov
	@find . -maxdepth 1 -type d \( -name "Q_[0-9]*" -o -name "home" -o -name "Desktop" -o -name "C++_Amit" -o -name "OS_Final_Versions" -o -name "OS_final" \) -exec rm -rf {} +
	@echo "[COVER] cleaned"


	@echo "[COVER] cleaned"

# ---- SERVER coverage ----
cover_server: clean_coverage 
	@echo "[COVER] compile server objects WITH coverage"
	g++ $(COVER_CXXFLAGS) -DGCOV_MODE=1 -c server.cpp -o server.o
	g++ $(COVER_CXXFLAGS) -c ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/Server_Socket.o
	g++ $(COVER_CXXFLAGS) -c pipeline/Pipeline.cpp -o pipeline/Pipeline.o
	@echo "[COVER] link server_cov"
	g++ $(COVER_LDFLAGS) server.o ../Q_7/Socket_class/Server_Socket.o pipeline/Pipeline.o -o server_cov

	@echo "[COVER] purge old .gcda near .gcno"
	@rm -f server.gcda ../Q_7/Socket_class/*.gcda pipeline/*.gcda

# before running (no GCOV_PREFIX; write in-place)
	@echo "[COVER] run server; exercise with your client, then exit gracefully"
	./server_cov || true

	@echo "[COVER] gcov (focused sources)"
	@gcov -b -c -r -o .                      server.cpp
	@gcov -b -c -r -o ../Q_7/Socket_class    ../Q_7/Socket_class/Server_Socket.cpp
	@gcov -b -c -r -o pipeline               pipeline/Pipeline.cpp
	@echo "== GCOV (server) DONE =="

# ---- CLIENT coverage (runs a background server) ----
cover_client: clean_coverage
	@echo "[COVER] build server_cov (no GCOV_MODE defines needed)"
	g++ $(COVER_CXXFLAGS) -c server.cpp -o server.o
	g++ $(COVER_CXXFLAGS) -c ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/Server_Socket.o
	g++ $(COVER_CXXFLAGS) -c pipeline/Pipeline.cpp -o pipeline/Pipeline.o
	g++ $(COVER_LDFLAGS) server.o ../Q_7/Socket_class/Server_Socket.o pipeline/Pipeline.o -o server_cov

	@echo "[COVER] build client_cov"
	g++ $(COVER_CXXFLAGS) -c client.cpp -o client.o
	g++ $(COVER_CXXFLAGS) -c ../Q_7/Socket_class/Client_Socket.cpp -o ../Q_7/Socket_class/Client_Socket.o
	g++ $(COVER_LDFLAGS) client.o ../Q_7/Socket_class/Client_Socket.o -o client_cov

	@echo "[COVER] start server in background"
	@( ./server_cov >/dev/null 2>&1 & echo $$! > .srvpid )
	@sleep 1

	@echo "[COVER] run client"
	@if [ -f coverage.in ]; then ./client_cov < coverage.in; else ./client_cov; fi

	@echo "[COVER] stop server"
	@kill -INT $$(cat .srvpid) 2>/dev/null || true
	@wait $$(cat .srvpid) 2>/dev/null || true
	@rm -f .srvpid

	@echo "[COVER] collect .gcda and run gcov"
	@rm -rf .cov && mkdir -p .cov
	@$(GCOV_ENV) true # no-op; kept to avoid touching other logic
	@find . -name '*.gcda' -maxdepth 2 -exec cp -f {} . \; 2>/dev/null || true
	@find ../Q_7/Socket_class -name '*.gcda' -maxdepth 1 -exec true \; 2>/dev/null || true

	@gcov -b -c -r -o .                   client.cpp
	@gcov -b -c -r -o ../Q_7/Socket_class ../Q_7/Socket_class/Client_Socket.cpp
	@echo "== GCOV (client) DONE =="

%:
	@:



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
	@rm -f pipeline/*.o
