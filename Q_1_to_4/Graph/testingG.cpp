#include "../Graph/Graph.hpp"
using namespace Graph_implementation;
#define DIRECTED truel

int main(){

Graph<int> g(4);
g.add_edge(0, 1, 10);
g.add_edge(1, 2, 1);
g.add_edge(2, 1, 1);  // This cycle can trap the algorithm
g.add_edge(2, 3, 10);
g.add_edge(0, 3, 5);  // Parallel path (to confuse augmenting path logic)

double maxflow = g.edmon_karp_algorithm(0, 3);
std::cout << "Max Flow: " << maxflow << std::endl;



return 0;

}