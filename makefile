# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./Q_1_to_5/Graph

# Source files
SRCDIR = Q_1_to_5
GRAPH_DIR = $(SRCDIR)/Graph
SRC = $(GRAPH_DIR)/Graph.cpp $(SRCDIR)/main_graph.cpp

# Output
TARGET = main_graph

# Build rule
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# Clean rule
clean:
	rm -f $(TARGET)
