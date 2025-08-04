#pragma once
#include "AlgoIO.hpp"
#include <vector>
#include <string>
#include <sstream>

// Strategy for computing strongly-connected components via Kosaraju's algorithm
// Request<T> is assumed to carry a ready-to-use graph

template <typename T>
class SCC_Algo : public AlgorithmIO<T> {
public:
    Response run(const Request<T>& req) override {

        std::vector<std::vector<T>> scc_res = req.graph.kosarajus_algorithm_scc();

       
        std::ostringstream oss;
        oss << scc_res;
        return {true, oss.str()};
    }
};

// Overload operator<< for any ostream to print a list of SCCs
// Each component is printed as {v1 v2 ...}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<T>>& scc_list) {
    os << "{\n";
    for (const auto& component : scc_list) {
        os << "  { ";
        for (const auto& vertex : component) {
            os << vertex << " ";
        }
        os << "}\n";
    }
    os << "}";
    return os;
}
