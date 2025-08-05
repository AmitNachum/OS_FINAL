#pragma once
#include <vector>
#include <memory>
#include <sstream>
#include <string>


// Strategy for computing an Eulerian circuit
// Request<T> is assumed to carry a ready-to-use graph with
// a method `std::unique_ptr<std::vector<T>> find_euler_circuit();`

template <typename T>
class EulerAlgo : public AlgorithmIO<T> {
public:
    virtual Response run(const Request<T>& req) override {
        // Attempt to find an Eulerian circuit
        std::unique_ptr<std::vector<T>> circuit = req.graph.find_euler_circut();
        if (!circuit || circuit->empty()) {
            return {false, "Graph is not Eulerian"};
        }

       
        std::ostringstream oss;
        for (size_t i = 0; i < circuit->size(); ++i) {
            oss << (*circuit)[i];
            if (i != circuit->size() - 1)
                oss << " ";
        }

        return {true, oss.str()};
    }
};

// Overload operator<< for any ostream to print a vector<T>
// e.g. an Eulerian circuit or any sequence of vertices

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& cycle) {
    os << "{ ";
    for (const auto& v : cycle) {
        os << v << " ";
    }
    os << "}";
    return os;
}
