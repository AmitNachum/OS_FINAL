#pragma once


// Strategy for finding a Hamiltonian cycle
// T must support operator<< for serialization

template <typename T>
class HamiltonAlgo : public AlgorithmIO<T> {
public:
     virtual Response run(const Request<T>& req) override {
        if(!req.start) 
           return {false,"Missing start"};

        const T& first = *req.start;
        std::vector<T> cycle = req.graph.hamilton_cycle(first);
        if (cycle.empty()) {
            return {false, "No Cycle was detected"};
        }

        // serialize the cycle
        std::ostringstream oss;
        oss << "{";
        for (const auto& vertex : cycle) {
            oss << vertex << " ";
        }
        oss << "}";

        return {true, oss.str()};
    }
};
