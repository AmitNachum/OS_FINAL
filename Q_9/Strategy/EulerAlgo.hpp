#pragma once
#include "AlgoIO.hpp"     // AlgorithmIO<T>, Request<T>, Response
#include <vector>
#include <memory>
#include <sstream>
#include <string>
#include <ostream>

// Strategy for computing an Eulerian circuit.
// The graph must expose a unified API:
//   std::vector<T> euler_circuit() const;
// which dispatches internally based on graph directedness.
template <typename T>
class EulerAlgo : public AlgorithmIO<T> {
public:
    Response run(const Request<T>& req) override {
        // Ask the graph for an Eulerian circuit (directed or undirected).
        std::vector<T> cycle = req.graph.euler_circuit();

        if (cycle.empty()) {
            // No Eulerian circuit exists.
            return {false, "Graph is not Eulerian"};
        }

        // Format the circuit as "{ v1 v2 ... }"
        std::ostringstream oss;
        oss << "{ ";
        for (const auto& v : cycle) {
            oss << v << " ";
        }
        oss << "}";

        return {true, oss.str()};
    }
};

// Optional pretty-printer for vectors (e.g., to print circuits directly).
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& cycle) {
    os << "{ ";
    for (const auto& v : cycle) {
        os << v << " ";
    }
    os << "}";
    return os;
}
