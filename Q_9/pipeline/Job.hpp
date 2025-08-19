#pragma once
#include <string>
#include <optional>
#include <memory>

// Correct path to your Graph
#include "../../Q_1_to_4/Graph/Graph.hpp"

// Mirror the aliases used in server.cpp
namespace GI = Graph_implementation;
using Vertex = int;
using GraphT = GI::Graph<Vertex>;

// Job represents a single client request ready for algorithm processing.
namespace Q9 {

struct Job {
    int client_fd = -1;                 // Target socket FD for response
    std::string job_id;                 // Unique identifier for fan-in
    std::shared_ptr<GraphT> graph;      // Shared graph instance
    std::optional<int> s;               // Max-Flow source (if provided)
    std::optional<int> t;               // Max-Flow sink (if provided)
    bool directed = true;               // Whether the graph is directed

    Job() = default;                    // Default constructor

};

} // namespace Q9
