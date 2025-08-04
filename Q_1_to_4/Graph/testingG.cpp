#include "../Graph/Graph.hpp"
using namespace Graph_implementation;
#define DIRECTED truel

int main() {
    // 1) Graph WITH a Hamiltonian cycle: 0–1–2–3–0
    Graph<int> g1(4);
    g1.add_edge(0, 1, 1);
    g1.add_edge(1, 2, 1);
    g1.add_edge(2, 3, 1);
    g1.add_edge(3, 0, 1);

    auto cycle1 = g1.hamilton_cycle(0);
    if (cycle1.empty()) {
        std::cout << "g1: no Hamiltonian cycle found\n";
    } else {
        std::cout << "g1 cycle:";
        for (int v : cycle1) std::cout << ' ' << v;
        std::cout << "\n";
    }

    // 2) Graph WITHOUT a Hamiltonian cycle: a simple path 0–1–2
    Graph<int> g2(3);
    g2.add_edge(0, 1, 1);
    g2.add_edge(1, 2, 1);

    auto cycle2 = g2.hamilton_cycle(0);
    if (cycle2.empty()) {
        std::cout << "g2: no Hamiltonian cycle found\n";
    } else {
        std::cout << "g2 cycle:";
        for (int v : cycle2) std::cout << ' ' << v;
        std::cout << "\n";
    }

    return 0;
}