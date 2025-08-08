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

# Phony helpers (avoid circular names)
.PHONY: build-server build-client clean print
build-server: $(SERVER_EXE)
build-client: $(CLIENT_EXE)

print:
	@echo "SERVER_SRCS:"; echo $(SERVER_SRCS) | tr ' ' '\n'
	@echo ""; echo "CLIENT_SRCS:"; echo $(CLIENT_SRCS) | tr ' ' '\n'
	@echo ""; echo "INCLUDES:"; echo $(INCLUDES)

clean:
	@echo "Cleaning Stage 9 build artifacts..."
	@rm -rf $(OBJ_DIR) $(SERVER_EXE) $(CLIENT_EXE)
