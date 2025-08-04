#pragma once
#include <memory>
#include <string>
#include <algorithm>
#include "Q_7/Strategy/Strategy_interface.hpp"

// Factory for creating algorithm strategies based on request name
// T is the vertex/key type used in Graph<T>

template <typename T>
class AlgorithmsFactory {
public:
    static std::unique_ptr<AlgorithmIO<T>> create(const Request<T>& req) {
        // normalize name to lowercase
        std::string name = req.name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (name == "hamilton") {
            return std::make_unique<HamiltonAlgo<T>>();
        }
        else if (name == "euler cycle" || name == "eulerian circuit") {
            return std::make_unique<EulerAlgo<T>>();
        }
        else if (name == "scc" || name == "strongly connected components") {
            return std::make_unique<SCC_Algo<T>>();
        }
        else if (name == "maxflow" || name == "edmonds-karp") {
            return std::make_unique<MaxFlowAlgo<T>>();
        }
        else if (name == "mst" || name == "prim") {
            return std::make_unique<MST_Algo<T>>();
        }
        // unknown algorithm
        return nullptr;
    }
};
