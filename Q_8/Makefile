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

# ---------- Coverage (Q_8) ----------
.PHONY: cover_server cover_client clean_coverage clean

# Clean coverage artifacts anywhere (Q_8 and the reused Q_7 sockets)
clean_coverage:
	@find . -type f \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) -delete
	@find ../Q_7 -type f \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) -delete
	@rm -f server_cov client_cov .srvpid
	@echo "[COVER] cleaned"
# -------------------- SERVER --------------------
cover_server:
    # 1. Clean up old coverage files
	rm -f *.o *.gcno *.gcda *.gcov
	rm -f ../Q_7/Socket_class/*.o ../Q_7/Socket_class/*.gcno ../Q_7/Socket_class/*.gcda ../Q_7/Socket_class/*.gcov
    
    # 2. Compile server objects
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
		-I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
		-DGCOV_MODE=1 -c server.cpp -o server.o
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
		-I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
		-DGCOV_MODE=1 -c ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/Server_Socket.o
        
    # 3. Link executable
	g++ --coverage server.o ../Q_7/Socket_class/Server_Socket.o -pthread -o server_cov
    
    # 4. Run server
	./server_cov
    
    # 5. Generate gcov reports (FIXED)
	gcov -b -c -r server.cpp -o .
	gcov -b -c -r ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/
# -------------------- CLIENT --------------------
cover_client: clean_coverage
	@echo "[COVER] compile server objects WITH coverage (NO GCOV_MODE here)"
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -c server.cpp -o server.o
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -c ../Q_7/Socket_class/Server_Socket.cpp -o ../Q_7/Socket_class/Server_Socket.o
	@echo "[COVER] link server_cov"
	g++ --coverage server.o ../Q_7/Socket_class/Server_Socket.o -pthread -o server_cov

	@echo "[COVER] compile client objects WITH coverage (GCOV_MODE=1 is fine)"
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c client.cpp -o client.o
	g++ -O0 -g --coverage -std=c++17 -Wall -Wextra -pthread \
	    -I. -I../Q_1_to_4/Graph -I../Q_6 -I../Q_7 -I../Q_7/Factory -I../Q_7/Socket_class -I../Q_7/Strategy \
	    -DGCOV_MODE=1 -c ../Q_7/Socket_class/Client_Socket.cpp -o ../Q_7/Socket_class/Client_Socket.o
	@echo "[COVER] link client_cov"
	g++ --coverage client.o ../Q_7/Socket_class/Client_Socket.o -pthread -o client_cov

	@echo "[COVER] start server_cov in background"
	@( ./server_cov >/dev/null 2>&1 & echo $$! > .srvpid )
	@sleep 1.0

	@echo "[COVER] run client_cov (use coverage.in if present)"
	@if [ -f coverage.in ]; then ./client_cov < coverage.in; else ./client_cov; fi

	@echo "[COVER] stop background server"
	@kill -INT $$(cat .srvpid) 2>/dev/null || true
	@wait $$(cat .srvpid) 2>/dev/null || true
	@rm -f .srvpid

	@echo "[COVER] generate .gcov for client"
	- gcov -b -c -r -o client.o                                   client.cpp
	- gcov -b -c -r -o ../Q_7/Socket_class/Client_Socket.o        ../Q_7/Socket_class/Client_Socket.cpp
	@echo "== GCOV (client) DONE =="


# ---------- Cleanup ----------
clean: clean_coverage
	@rm -f server client server_cov client_cov *.o callgrind.*.out
	@echo "Clean complete."


clean:
	-lsof -t -i:5555 | xargs kill || true
	rm -f server client
	rm -f *.o *.gcno *.gcda *.gcov
	rm -f $(wildcard ../Q_7/Socket_class/*.o)
	rm -f $(wildcard ../Q_7/Socket_class/*.gcno)
	rm -f $(wildcard ../Q_7/Socket_class/*.gcda)
	rm -f $(wildcard ../Q_7/Socket_class/*.gcov)
	rm -f server_cov client_cov
	rm -f callgrind.callgrind_server.out