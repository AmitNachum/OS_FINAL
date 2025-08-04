#pragma once
#include "AlgoIO.hpp"
#include <vector>
#include <string>
#include <optional>
#include <sstream>

// Strategy for computing an MST via Prim's algorithm
// Request<T> is assumed to have std::optional<T> start

template <typename T>
class MSTAlgo : public AlgorithmIO<T> {
public:
    Response run(const Request<T>& req) override {
        if (!req.start.has_value()) {
            return {false, "Missing the starting vertex"};
        }
        const T& first = *req.start;
        
        auto edges_list = req.graph.prims_algorithm(first);

        // Serialize the edge list
        std::ostringstream oss;
        oss << edges_list;
        return {true, oss.str()};
    }
};

// Streaming operator for a list of edges
// Outputs: { (u,v,weight: w) ... }
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<Edge<T>>& edges_list) {
    os << "{\n";
    for (const auto& edge : edges_list) {
        os << "("
           << edge.vertex_r << ", "
           << edge.vertex_w << ", weight: "
           << edge.edge_weight << ")\n";
    }
    os << "}\n";
    return os;
}

