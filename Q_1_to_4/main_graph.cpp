#include <iostream>
#include "Graph/Graph.hpp"
#include <random>
#include <thread>
#include <memory>

using std::cout;
using std::endl;
using namespace Graph_implementation;

int main() {
    cout << "=== Generating Graph ===" << endl;

    // Generate random number of vertices between 4 and 8 (potentially Eulerian)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(4, 8);
    size_t n = static_cast<size_t>(dist(gen));

    Graph<int> G(n);

    // Add n vertices labeled 1..n
    for (int i = 1; i <= static_cast<int>(n); ++i) {
        G.add_vertex(i);
    }

    // Connect vertices in a cycle 1-2-...-n-1 to make degrees even
    for (int i = 1; i <= static_cast<int>(n); ++i) {
        int u = i;
        int v = (i % static_cast<int>(n)) + 1;
        G.add_edge(u, v, 1);
    }

    cout << "\n----- Graph Representation -----" << endl;
    cout << G << endl;

    cout << "\n=== Eulerian Check ===" << endl;
    if (G.is_eulerian()) {
        cout << "Graph is Eulerian!" << endl;
    } else {
        cout << "Graph is NOT Eulerian!" << endl;
    }

    try {
        cout << "\n=== Finding Euler Circuit ===" << endl;
        // euler_circuit() returns std::vector<int>, not a unique_ptr
        std::vector<int> circuit = G.euler_circuit();
        cout << "Euler Circuit: ";
        for (int v : circuit) {
            cout << v << ' ';
        }
        cout << endl;
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    return 0;
}
