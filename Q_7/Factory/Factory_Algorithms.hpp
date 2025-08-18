#pragma once
#include "../Strategy/AlgoIO.hpp"
#include "../../Q_7/Strategy/Strategy_interface.hpp"

#include <memory>
#include <string>
#include <algorithm>
#include <syncstream>
#define SCOUT std::osyncstream(std::cout)




//Custom Factory for generating The requested algorithm

template <typename T>
class AlgorithmsFactory {
public:
    static std::unique_ptr<AlgorithmIO<T>> create(const Request<T>& req) {
        // normalize name to lowercase
        std::string name = req.name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c){ return std::tolower(c); });

                      SCOUT << "Creating algorithm: " << name << std::endl;


        if (name == "hamilton") {
            return std::make_unique<HamiltonAlgo<T>>();
        }
        else if (name == "euler cycle" || name == "eulerian circuit" || name == "euler") {
            return std::make_unique<EulerAlgo<T>>();
        }
        else if (name == "scc" || name == "strongly connected components") {
            return std::make_unique<SCC_Algo<T>>();
        }
        else if (name == "maxflow" || name == "edmonds-karp") {
            return std::make_unique<MaxFlow<T>>();
        }
        else if (name == "mst" || name == "prim") {
            return std::make_unique<MSTAlgo<T>>();
        }
       
        return nullptr;
    }
};
