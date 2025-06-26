# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./Graph -g -fprofile-arcs -ftest-coverage
LDFLAGS = #-fprofile-arcs -ftest-coverage
#Dir and Files
GRAPH_DIR = Graph
SRC = main_graph.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = main_graph

.PHONY: all coverage clean

all: $(TARGET)
# Build target
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run and generate coverage
coverage: all
	@./$(TARGET)
	@gcov -b -c main_graph.cpp Graph/Graph.cpp
	@find . -type f -name '*.gcov' \
	! -name 'main_graph.cpp.gcov' \
	! -name 'Graph.cpp.gcov' \
	-delete

# Clean build and coverage files
clean:
	rm -f $(TARGET) $(OBJ) \
	*.gcno *.gcda *.gcov \
	 $(GRAPH_DIR)/*.gcno $(GRAPH_DIR)/*.gcda $(GRAPH_DIR)/*.gcov
