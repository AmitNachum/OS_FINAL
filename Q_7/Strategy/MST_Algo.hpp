#pragma once
#include <vector>
#include <string>
#include <optional>
#include <sstream>

// Strategy for computing an MST via Prim's algorithm (or arborescence if directed)
// Request<T> is assumed to have std::optional<T> start

template <typename T>
class MSTAlgo : public AlgorithmIO<T> {
public:
    virtual Response run(const Request<T>& req) override {
        if (!req.start.has_value()) {
            return {false, "Missing the starting vertex"};
        }
        const T& first = *req.start;

        // For undirected graphs this returns a Prim MST.
        // For directed graphs this returns a minimum arborescence (rooted at 'first').
        auto edges_list = req.graph.prims_algorithm(first);

        if (edges_list.empty()) {
            return {false, "No spanning tree/arborescence found from the given root"};
        }

        // Serialize the edge list
        std::ostringstream oss;
        oss << edges_list;
        return {true, oss.str()};
    }
};

// Streaming operator for a list of edges
// IMPORTANT: print as (from, to, weight: w) i.e., (vertex_w -> vertex_r)
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<Edge<T>>& edges_list) {
    os << "{\n";
    for (const auto& edge : edges_list) {
        os << "("
           << edge.vertex_w << ", "
           << edge.vertex_r << ", weight: "
           << edge.edge_weight << ")\n";
    }
    os << "}\n";
    return os;
}
