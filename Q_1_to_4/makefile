# =========================
# Compiler and flags
# =========================
CXX = g++

# Coverage/debug toolchain (used for main and LIGHT tests)
CXXFLAGS_COV = -std=c++17 -Wall -Wextra -I./Graph -g --coverage
LDFLAGS_COV  = --coverage

# Fast/optimized toolchain (used for FULL/HARD tests)
CXXFLAGS_FAST = -std=c++17 -Wall -Wextra -I./Graph -O2 -DNDEBUG
LDFLAGS_FAST  =

# Optional user extras (e.g. make CXXEXTRA='-DPERF_MS_LIMIT=16000 -DPERF_SIZE_SCALE=0.8')
CXXEXTRA ?=

# =========================
# Sources
# =========================
GRAPH_DIR = Graph
MAIN_SRC  = main_graph.cpp
TEST_SRC  = Graph/testingG.cpp

ALL_GRAPH_SRCS = $(wildcard $(GRAPH_DIR)/*.cpp)
# exclude the test file from library sources
LIB_SRCS       = $(filter-out $(TEST_SRC), $(ALL_GRAPH_SRCS))

MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
LIB_OBJS = $(LIB_SRCS:.cpp=.o)

TARGET     = main_graph
TEST_LIGHT = test_graph_light   # coverage, no perf tests
TEST_FULL  = test_graph_full    # fast, all heavy/perf

# Coverage to compute ONLY on your sources (not STL, not test file)
GCOV_SRCS = $(MAIN_SRC) $(LIB_SRCS)

# .gcov files we KEEP (everything else deleted after gcov)
GCOV_KEEP = main_graph.cpp.gcov Graph.hpp.gcov

# HTML report tools/dir
LCOV       = lcov
GENHTML    = genhtml
REPORT_DIR = coverage_report

VPATH = .:$(GRAPH_DIR)

.PHONY: all Main test test-hard coverage report clean

# =========================
# Default target
# =========================
all: Main

# =========================
# Build rules
# =========================
# main (with coverage)
$(TARGET): $(MAIN_OBJ) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS_COV) $(CXXEXTRA) $^ -o $@ $(LDFLAGS_COV)

# LIGHT tests object (coverage + disable perf tests completely)
Graph/testingG.light.o: $(TEST_SRC)
	$(CXX) $(CXXFLAGS_COV) $(CXXEXTRA) -DHEAVY_TESTS=0 -DENABLE_PERF_TESTS=0 -DVERY_HEAVY_TESTS=0 -c $< -o $@

# LIGHT tests binary (coverage)
$(TEST_LIGHT): Graph/testingG.light.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS_COV) $(CXXEXTRA) $^ -o $@ $(LDFLAGS_COV)

# FULL/HARD tests object (FAST, enable ALL perf/heavy)
$(TEST_OBJ): $(TEST_SRC)
	$(CXX) $(CXXFLAGS_FAST) $(CXXEXTRA) -DHEAVY_TESTS=1 -DENABLE_PERF_TESTS=1 -DVERY_HEAVY_TESTS=1 -c $< -o $@

# FULL/HARD tests binary (FAST, no coverage)
$(TEST_FULL): $(TEST_OBJ) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS_FAST) $(CXXEXTRA) $^ -o $@ $(LDFLAGS_FAST)

# Generic rule (coverage toolchain for main + library sources)
%.o: %.cpp
	$(CXX) $(CXXFLAGS_COV) $(CXXEXTRA) -c $< -o $@

# =========================
# Run only main
# =========================
Main: $(TARGET)
	@echo "Running main program..."
	@./$(TARGET)

# =========================
# 1) make test  -> LIGHT (Valgrind + Coverage)   [NO heavy/perf]
# =========================
# Skip Valgrind via: make test TEST_NO_VALGRIND=1
test: $(TEST_LIGHT) $(TARGET)
	@echo "Running LIGHT tests (coverage, NO perf/heavy)..."
ifeq ($(TEST_NO_VALGRIND),1)
	@./$(TEST_LIGHT)
else
	@valgrind --leak-check=full --error-exitcode=1 ./$(TEST_LIGHT)
endif
	@echo "Running main (adds to coverage)..."
	@./$(TARGET)
	@echo "Generating coverage for YOUR sources only..."
	@find . -type f -name '*.gcov' -delete
	@gcov -b -c $(GCOV_SRCS) >/dev/null 2>&1 || true
	@# keep only selected .gcov files, delete the rest (STL noise etc.)
	@for f in *.gcov; do \
	   keep=0; for k in $(GCOV_KEEP); do [ "$$f" = "$$k" ] && keep=1; done; \
	   [ $$keep -eq 1 ] || rm -f "$$f"; \
	 done
	@echo "Coverage ready: $(GCOV_KEEP)"
	@echo "Calculating total coverage summary..."
	@$(LCOV) --capture --directory . --output-file coverage.info >/dev/null 2>&1 || true
	@$(LCOV) --remove coverage.info '/usr/*' '*/include/*' '*/c++/*' '*/doctest.h' '*/Graph/testingG.cpp' -o coverage.filtered.info >/dev/null 2>&1 || true
	@# stable 'sed' parsing (avoids awk dialect issues)
	@$(LCOV) --summary coverage.filtered.info 2>/dev/null | sed -n \
	  -e 's/.*lines\.*: *\([0-9.]\+%\).*/Line coverage:     \1/p' \
	  -e 's/.*functions\.*: *\([0-9.]\+%\).*/Function coverage: \1/p' \
	  -e 's/.*branches\.*: *\([0-9.]\+%\).*/Branch coverage:   \1/p'

# =========================
# 2) make test-hard  -> FULL (ALL heavy/perf + VERY_HEAVY), no coverage
# =========================
test-hard: $(TEST_FULL)
	@echo "Running FULL/HARD tests (NO coverage, -O2, includes perf + very heavy)..."
	@./$(TEST_FULL)

# =========================
# Extra: coverage on main only (optional)
# =========================
coverage: $(TARGET)
	@./$(TARGET)
	@find . -type f -name '*.gcov' -delete
	@gcov -b -c $(MAIN_SRC) $(LIB_SRCS) >/dev/null 2>&1 || true
	@for f in *.gcov; do \
	   keep=0; for k in $(GCOV_KEEP); do [ "$$f" = "$$k" ] && keep=1; done; \
	   [ $$keep -eq 1 ] || rm -f "$$f"; \
	 done
	@echo "Coverage ready: $(GCOV_KEEP)"

# =========================
# HTML coverage (filtered) — runs tests first
# =========================
report: test
	@echo "Creating filtered HTML coverage report..."
	@$(LCOV) --capture --directory . --output-file coverage.info >/dev/null 2>&1 || true
	@$(LCOV) --remove coverage.info '/usr/*' '*/include/*' '*/c++/*' '*/doctest.h' '*/Graph/testingG.cpp' -o coverage.filtered.info >/dev/null 2>&1 || true
	@rm -rf $(REPORT_DIR)
	@$(GENHTML) coverage.filtered.info --output-directory $(REPORT_DIR) >/dev/null 2>&1 || true
	@echo "Open $(REPORT_DIR)/index.html"

# =========================
# Clean
# =========================
clean:
	rm -f $(TARGET) $(TEST_LIGHT) $(TEST_FULL) \
	      $(MAIN_OBJ) $(TEST_OBJ) Graph/testingG.light.o $(LIB_OBJS) \
	      *.gcno *.gcda *.gcov coverage.info coverage.filtered.info
	rm -rf $(REPORT_DIR)
