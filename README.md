# OS Final Project

## Overview

This project is the **final assignment** for the Operating Systems course.
It implements a **multi-stage, multi-protocol networking system** written in C++, showcasing advanced OS concepts such as socket programming, concurrency, and design patterns.
The project is divided into multiple stages (`Q_1` to `Q_9`), each building on the previous one to expand functionality.

---

## Project Goals

* **Implement network communication** using both:

  * **TCP/UDP (INET)** sockets
  * **UNIX domain sockets** (stream and datagram)
* **Support multiple algorithms** for graph processing, such as:

  * Eulerian paths/cycles
  * Minimum Spanning Tree (MST)
  * Hamiltonian cycles
  * Maximum Flow
  * Strongly Connected Components (SCC)
* **Demonstrate OS design patterns**:

  * Reactor Pattern (event-driven)
  * Proactor Pattern (asynchronous operations)
* **Integrate GCOV coverage support** for testing and profiling.

---

## Features

1. **Client-Server Architecture**

   * Server listens for incoming connections (TCP, UDP, UDS).
   * Clients can connect and send algorithm requests.
   * Supports both **interactive mode** and **scripted input** for automation.

2. **Graph Algorithms**

   * Built-in algorithms implemented in `Q_1_to_4` and `Q_7` modules.
   * Server can parse a graph description from client input and run selected algorithms.
   * Results returned to the client in a serialized format.

3. **Pipeline Processing**

   * Modular `Pipeline` system in `Q_9` to manage algorithm execution.
   * Separation of job creation, execution, and result handling.

4. **Coverage Mode**

   * When built with `GCOV_MODE=1`, server and client are instrumented for **code coverage analysis**.
   * Results collected via `gcov` after test runs.

5. **Design Patterns**

   * **Factory Pattern**: Dynamically create algorithm objects.
   * **Strategy Pattern**: Allow interchangeable algorithm implementations.
   * **Reactor Pattern**: Handle multiple I/O events without blocking.
   * **Proactor Pattern**: Asynchronous task handling per connection.

---

## Directory Structure

```
OS_final/
├── Q_1_to_4/          # Graph data structures & base algorithms
├── Q_6/               # Reactor Pattern implementation
├── Q_7/               # Factory, Strategy, and Socket classes
├── Q_8/               # Extended server/client logic with coverage support
├── Q_9/               # Final integrated pipeline & coverage scripts
└── Makefile           # Build and coverage automation
```

---

## Building and Running

### Top-Level Makefile

From the root:

```bash
make all                 # build all stages
make clean               # clean all builds
make rebuild             # clean + rebuild
make cover_server_all    # run coverage server builds in all stages
make valgrind_server_all # run valgrind on all servers
make helgrind_server_all # run helgrind on all servers
make callgrind_server_all# run callgrind on all servers
```

---

### Stage-Specific Instructions

#### Q_1_to_4 (Graph Algorithms)

```bash
cd Q_1_to_4
make            # build main_graph
make test       # lightweight tests with Valgrind + coverage
make test-hard  # heavy/performance tests (no coverage)
make helgrind   # race detection
make callgrind  # profiling
make coverage   # generate .gcov
make report     # HTML report in coverage_report/
```

#### Q_6 / Q_7 / Q_8 / Q_9 (Servers & Clients)

For every stage after Q_1_to_4, follow this exact order:

```bash
cd Q_6   # or Q_7 / Q_8 / Q_9

# 1) Build
make

# 2) Memory checks
make valgrind_server
make valgrind_client ARGS="..."

# 3) Profiling
make callgrind_server
make callgrind_client ARGS="..."

# 4) Race detection
make helgrind_server
make helgrind_client ARGS="..."

# 5) Coverage build
make cover_build

# 6) Coverage execution
make cover_server
make cover_client

# 7) Generate gcov reports
make gcov_report
```

---

## Usage Example

1. Start the server:

```bash
./server
```

2. Start the client and enter commands:

```
ADD 5
euler
mst
```

3. Receive results from server.

---

## Requirements

* **g++** with C++17 support (C++20 in later stages)
* Linux/UNIX environment
* `gcov`, `lcov`, `genhtml` for coverage analysis
* `make` for build automation

---

## Author

**Amit Nachum** and **Or Bibi** – Ariel University  
*Operating Systems Final Project – 2025*
