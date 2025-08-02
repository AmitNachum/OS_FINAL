#include "../Graph/Graph.hpp"
using namespace Graph_implementation;
#define DIRECTED true

int main() {
    Graph<int> graph(5);

    // Add all vertices
    graph.add_vertex(1);
    graph.add_vertex(2);
    graph.add_vertex(3);
    graph.add_vertex(4);
    graph.add_vertex(5);

    // Cycle: 1 → 2 → 3 → 1
    graph.add_edge(1, 2, 1.0, DIRECTED);
    graph.add_edge(2, 3, 1.0, DIRECTED);
    graph.add_edge(3, 1, 1.0, DIRECTED);

    // Chain out of the cycle: 3 → 4 → 5
    graph.add_edge(3, 4, 1.0, DIRECTED);
    graph.add_edge(4, 5, 1.0, DIRECTED);

    // Run Kosaraju
    auto scc = graph.kosarajus_algorithm_scc();

    // Print SCCs
    std::cout << "{\n";
    for (const auto& component : scc) {
        std::cout << "[ ";
        for (const auto& vertex : component) {
            std::cout << vertex << " ";
        }
        std::cout << "]\n";
    }
    std::cout << "}\n";

    return 0;
}
