#include <iostream>
#include "Graph/Graph.hpp"
#include <random>
#include <thread>
#include <memory>
#include <getopt.h>

using std::cout;
using std::endl;
using namespace Graph_implementation;

int main(int argc, char* argv[]) {

    int v = -1;
    int e = -1;
    int random_seed = -1;
    int opt;

    while ((opt = getopt(argc, argv, "v:e:r:")) != -1) {
        switch (opt) {
            case 'v':
                v = atoi(optarg);
                break;
            case 'e':
                e = atoi(optarg);
                break;
            case 'r':
                random_seed = atoi(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-v <num_of_vertices(int)> -e <num_of_edeges(int)> -r <seed for?(int)>] " << endl;
                return 1;
        }
    }
    cout << "=== Generating Graph ===" << endl;

    if (v <= 0 || e <= 0 || random_seed < 0) {
        std::cerr << "Invalid parameters! Please provide positive integers for vertices and edges, and a non-negative seed." << endl;
        return 1;
    }
    if (e > v * (v - 1) / 2) {
        std::cerr << "Too many edges! For " << v << " vertices, the maximum number of edges is " << v * (v - 1) / 2 << "." << endl;
        return 1;
    }
    Graph<int> G(v, false); // Create an undirected graph with v vertices
    
    std::mt19937 rng(random_seed);
    std::uniform_int_distribution<int> dist(0, v - 1);
    int added_edges = 0;
    std::set<std::pair<int, int>> edge_set;
    int weight = 1; // Weight for edges, can be adjusted

    while (added_edges < e) {
        int u = dist(rng);
        int w = dist(rng);
        if (u == w) continue; // No self-loops
        auto edge = std::minmax(u, w);
        if (edge_set.count(edge)) continue; // No duplicate edges
        G.add_edge(u, w, weight);
        edge_set.insert(edge);
        ++added_edges;
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
