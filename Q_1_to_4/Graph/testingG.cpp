// test_graph_doctest.cpp
// Build: g++ -std=c++17 -O2 test_graph_doctest.cpp -o tests
// Run  : ./tests -s

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#ifndef GRAPH_HEADER
// Adjust this macro via -DGRAPH_HEADER="\"YourHeader.hpp\"" if needed.
#define GRAPH_HEADER "Graph.hpp"
#endif

#include GRAPH_HEADER

#include <chrono>
#include <random>
#include <numeric>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <limits>
#include <sstream>
#include <string>
#include <algorithm>

using namespace Graph_implementation;

// ============================== Global Configuration Macros ==============================
// Toggle perf tests entirely. In the Makefile we set -DENABLE_PERF_TESTS=0 for coverage runs.
#ifndef ENABLE_PERF_TESTS
#define ENABLE_PERF_TESTS 1
#endif

// Tweakable via -D when compiling
#ifndef PERF_SIZE_SCALE
#define PERF_SIZE_SCALE 1    // multiply sizes to scale up/down heavy tests
#endif

#ifndef HEAVY_TESTS
#define HEAVY_TESTS 1        // set to 0 to disable heavy/performance tests
#endif

#ifndef VERY_HEAVY_TESTS
#define VERY_HEAVY_TESTS 0   // set to 1 for extreme sizes (use with care)
#endif

// Smart default for time limit: faster bound with optimizations, looser otherwise.
// If you pass -DPERF_MS_LIMIT=... it will override this.
#ifndef PERF_MS_LIMIT
#  ifdef __OPTIMIZE__
#    define PERF_MS_LIMIT 12000
#  else
#    define PERF_MS_LIMIT 30000
#  endif
#endif

// Helper macro to scale sizes consistently
#define SZ(x) static_cast<int>((x) * PERF_SIZE_SCALE)

// ============================== Utility Builders & Helpers ==============================

template<typename T=int>
Graph<T> make_empty_graph(bool directed=false) {
    Graph<T> g(0, directed);
    return g;
}

template<typename T=int>
Graph<T> make_path_graph(int n, bool directed=false, double w=1.0) {
    Graph<T> g(0, directed);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i+1<n;i++) g.add_edge(static_cast<T>(i), static_cast<T>(i+1), w);
    return g;
}

template<typename T=int>
Graph<T> make_cycle_graph(int n, bool directed=false, double w=1.0) {
    Graph<T> g(0, directed);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i<n;i++) {
        int j = (i+1)%n;
        g.add_edge(static_cast<T>(i), static_cast<T>(j), w);
        if (!directed) {
            // undirected add_edge already inserts both directions, so the line above is sufficient
        }
    }
    return g;
}

template<typename T=int>
Graph<T> make_star_graph(int center, int leaves, bool directed=false, double w=1.0) {
    Graph<T> g(0, directed);
    g.add_vertex(static_cast<T>(center));
    for (int i=0;i<leaves;i++) {
        T v = static_cast<T>(i + (center==0 ? 1 : 0));
        if (v == center) v = static_cast<T>(leaves + 1);
        g.add_vertex(v);
        g.add_edge(static_cast<T>(center), v, w);
    }
    return g;
}

// Complete graph K_n (undirected)
template<typename T=int>
Graph<T> make_complete_graph(int n, double w=1.0) {
    Graph<T> g(0, false);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i<n;i++) {
        for (int j=i+1;j<n;j++) {
            g.add_edge(static_cast<T>(i), static_cast<T>(j), w);
        }
    }
    return g;
}

// Directed cycle on n vertices
template<typename T=int>
Graph<T> make_directed_cycle(int n, double w=1.0) {
    Graph<T> g(0, true);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i<n;i++) {
        int j = (i+1)%n;
        g.add_edge(static_cast<T>(i), static_cast<T>(j), w);
    }
    return g;
}

// Random undirected Erdos–Renyi G(n,p)
template<typename T=int>
Graph<T> make_random_undirected(int n, double p, uint32_t seed=42, double wmin=1.0, double wmax=10.0) {
    Graph<T> g(0,false);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> pr(0.0,1.0);
    std::uniform_real_distribution<double> wdist(wmin,wmax);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i<n;i++) {
        for (int j=i+1;j<n;j++) {
            if (pr(rng) < p) g.add_edge(static_cast<T>(i), static_cast<T>(j), wdist(rng));
        }
    }
    return g;
}

// Random directed graph with probability p for each ordered pair (i->j), i!=j
template<typename T=int>
Graph<T> make_random_directed(int n, double p, uint32_t seed=1337, double wmin=1.0, double wmax=10.0) {
    Graph<T> g(0,true);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> pr(0.0,1.0);
    std::uniform_real_distribution<double> wdist(wmin,wmax);
    for (int i=0;i<n;i++) g.add_vertex(static_cast<T>(i));
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            if (i==j) continue;
            if (pr(rng) < p) g.add_edge(static_cast<T>(i), static_cast<T>(j), wdist(rng));
        }
    }
    return g;
}

// Layered flow network: s -> layer1 -> layer2 -> ... -> t
// Each layer has L nodes, capacities approx cap. Returns (g, source, sink).
template<typename T=int>
std::tuple<Graph<T>,T,T> make_layered_flow(int layers, int L, double cap=10.0) {
    Graph<T> g(0,true);
    int id=0;
    T s = static_cast<T>(id++); g.add_vertex(s);
    std::vector<std::vector<T>> layer(layers);
    for (int d=0; d<layers; ++d) {
        layer[d].reserve(L);
        for (int i=0;i<L;i++) {
            T v = static_cast<T>(id++);
            g.add_vertex(v);
            layer[d].push_back(v);
        }
    }
    T t = static_cast<T>(id++); g.add_vertex(t);

    for (auto v: layer[0]) g.add_edge(s,v,cap);
    for (int d=0; d+1<layers; ++d) {
        for (auto u: layer[d]) for (auto v: layer[d+1]) g.add_edge(u,v,cap);
    }
    for (auto u: layer.back()) g.add_edge(u,t,cap);
    return {std::move(g), s, t};
}

// Grid graph (undirected) of size r x c, 4-neighborhood
template<typename T=int>
Graph<T> make_grid_graph(int r, int c, double w=1.0) {
    Graph<T> g(0,false);
    auto id = [&](int i, int j){ return static_cast<T>(i*c + j); };
    for (int i=0;i<r;i++)
        for (int j=0;j<c;j++)
            g.add_vertex(id(i,j));
    for (int i=0;i<r;i++) {
        for (int j=0;j<c;j++) {
            if (i+1<r) g.add_edge(id(i,j), id(i+1,j), w);
            if (j+1<c) g.add_edge(id(i,j), id(i, j+1), w);
        }
    }
    return g;
}

// Helper: transform vector<vector<int>> to set-of-sets for robust comparison (order-insensitive)
static std::vector<std::set<int>> to_set_of_sets(const std::vector<std::vector<int>>& comps) {
    std::vector<std::set<int>> S;
    S.reserve(comps.size());
    for (auto& c : comps) S.emplace_back(c.begin(), c.end());
    std::sort(S.begin(), S.end(), [](const auto& a, const auto& b){
        if (a.size() != b.size()) return a.size() < b.size();
        return *a.begin() < *b.begin();
    });
    return S;
}

// ============================== Section: Basic Graph Operations ==============================

TEST_CASE("Basic: add_vertex, add_edge, degree, remove_edge (undirected)") {
    Graph<int> g = make_empty_graph<int>(false);
    g.add_vertex(1);
    g.add_vertex(2);
    g.add_edge(1,2,3.5);
    CHECK(g.degree(1) == 1);
    CHECK(g.degree(2) == 1);
    CHECK(g.is_directed() == false);

    // remove by exact (v,w)
    g.remove_edge(1,2);
    CHECK(g.degree(1) == 0);
    CHECK(g.degree(2) == 0);

    // add and remove when weight changed: robust fallback removes by target
    g.add_edge(1,2,4.0);
    CHECK(g.degree(1) == 1);
    g.remove_edge(1,2); // wrong weight, should still remove
    CHECK(g.degree(1) == 0);
}

TEST_CASE("Basic: add_edge implicitly adds missing vertices; start anchor is stable") {
    Graph<int> g = make_empty_graph<int>(false);
    g.add_edge(10, 11, 1.0); // both vertices were not explicitly added
    CHECK(g.degree(10) == 1);
    CHECK(g.degree(11) == 1);
    CHECK(g.get_first() == 10);
}

TEST_CASE("Basic: in/out degree in directed graph") {
    Graph<int> g = make_empty_graph<int>(true);
    g.add_vertex(0); g.add_vertex(1); g.add_vertex(2);
    g.add_edge(0,1,1.0);
    g.add_edge(2,1,1.0);

    CHECK(g.is_directed() == true);
    CHECK(g.out_degree(0) == 1);
    CHECK(g.in_degree(1) == 2);
    CHECK(g.out_degree(1) == 0);
}

TEST_CASE("Basic: self-loop handling and idempotency (undirected)") {
    Graph<int> g(0,false);
    g.add_vertex(7);
    g.add_edge(7,7,2.0); // self-loop in undirected context
    // In this representation, self-loop adds one entry (7, 2.0) under 7; degree counts it as 1
    CHECK(g.degree(7) == 1);
    // Removing with wrong weight should still remove
    g.remove_edge(7,7);
    CHECK(g.degree(7) == 0);
}

TEST_CASE("Basic: multiple add_edge calls to same neighbor do not create multi-edges") {
    Graph<int> g(0,false);
    g.add_edge(1,2,1.0);
    g.add_edge(1,2,1.0);
    g.add_edge(1,2,5.0); // different weight, but representation uses set of pairs -> only one (2,5.0) or (2,1.0) depending on insertion
    CHECK(g.degree(1) == 1);
}

// ============================== Section: Eulerian ==============================

TEST_CASE("Euler (undirected): triangle is Eulerian, path is not") {
    Graph<int> tri = make_empty_graph<int>(false);
    tri.add_vertex(0); tri.add_vertex(1); tri.add_vertex(2);
    tri.add_edge(0,1,1.0);
    tri.add_edge(1,2,1.0);
    tri.add_edge(2,0,1.0);
    CHECK(tri.is_eulerian() == true);
    auto c1 = tri.euler_circuit();
    CHECK(!c1.empty());
    CHECK(c1.front() == c1.back());

    Graph<int> path = make_path_graph<int>(3,false,1.0);
    CHECK(path.is_eulerian() == false);
    auto c2 = path.euler_circuit();
    CHECK(c2.empty());
}

TEST_CASE("Euler (undirected): even-degree but disconnected => false") {
    Graph<int> g(0,false);
    // Two disconnected cycles
    g.add_edge(0,1,1.0); g.add_edge(1,2,1.0); g.add_edge(2,0,1.0);
    g.add_edge(10,11,1.0); g.add_edge(11,12,1.0); g.add_edge(12,10,1.0);
    // All vertices have even degree but the graph is disconnected
    CHECK(g.is_eulerian() == false);
}

TEST_CASE("Euler (directed): directed cycle is Eulerian; in!=out breaks") {
    auto cyc = make_directed_cycle<int>(5, 1.0);
    CHECK(cyc.is_eulerian() == true);
    auto circ = cyc.euler_circuit();
    CHECK(!circ.empty());
    CHECK(circ.front() == circ.back());

    // Break in/out balance: add extra outgoing edge
    cyc.add_vertex(99);
    cyc.add_edge(0,99,1.0);
    CHECK(cyc.is_eulerian() == false);
}

TEST_CASE("Euler (directed): non-weakly-connected over non-zero-degree subgraph -> false") {
    Graph<int> g(0,true);
    g.add_edge(0,1,1.0); // component A
    g.add_edge(2,3,1.0); // component B
    CHECK(g.is_eulerian() == false);
}

// ============================== Section: MST (Prim) & Arborescence ==============================

TEST_CASE("MST (Prim): known small graph total weight") {
    Graph<int> g = make_empty_graph<int>(false);
    for (int v=0; v<5; ++v) g.add_vertex(v);
    g.add_edge(0,1,1.0);
    g.add_edge(0,2,3.0);
    g.add_edge(1,2,1.5);
    g.add_edge(1,3,2.0);
    g.add_edge(2,3,2.5);
    g.add_edge(3,4,0.5);

    auto mst = g.prims_algorithm(0);
    double sum = 0.0;
    for (auto& e : mst) sum += e.edge_weight;
    CHECK(mst.size() == 4);
    CHECK(sum == doctest::Approx(5.0));
}

TEST_CASE("MST (Prim): disconnected graph returns tree for source component only") {
    Graph<int> g(0,false);
    for (int v=0; v<6; ++v) g.add_vertex(v);
    // component A
    g.add_edge(0,1,1.0);
    g.add_edge(1,2,1.0);
    // component B
    g.add_edge(3,4,1.0);
    g.add_edge(4,5,1.0);

    auto mst0 = g.prims_algorithm(0);
    CHECK(mst0.size() == 2); // only spanning tree of component A (3 nodes -> 2 edges)

    auto mst3 = g.prims_algorithm(3);
    CHECK(mst3.size() == 2); // for component B
}

TEST_CASE("MST (Prim): negative weights (still valid for Prim)") {
    Graph<int> g(0,false);
    for (int v=0; v<4; ++v) g.add_vertex(v);
    g.add_edge(0,1,-1.0);
    g.add_edge(1,2,-2.0);
    g.add_edge(2,3, 1.0);
    g.add_edge(0,3, 2.0);
    auto mst = g.prims_algorithm(0);
    CHECK(mst.size() == 3);
    double s=0; for (auto& e : mst) s += e.edge_weight;
    CHECK(s < 0.5); // should include negative edges, sum < 0.5
}

TEST_CASE("Arborescence (Directed Chu–Liu/Edmonds): simple case") {
    Graph<int> g(0,true);
    for (int v=0; v<4; ++v) g.add_vertex(v);
    // root = 0
    g.add_edge(0,1,1.0);
    g.add_edge(0,2,5.0);
    g.add_edge(1,2,1.0);
    g.add_edge(1,3,4.0);
    g.add_edge(2,3,1.0);

    auto arb = g.prims_algorithm(0); // dispatch to directed_arborescence_impl
    CHECK(arb.size() == 3);
    std::unordered_map<int,int> indeg;
    for (auto& e : arb) indeg[e.vertex_r]++;
    CHECK(indeg[1] == 1);
    CHECK(indeg[2] == 1);
    CHECK(indeg[3] == 1);
}

TEST_CASE("Arborescence: unreachable nodes from root => expect empty result") {
    Graph<int> g(0,true);
    for (int v=0; v<5; ++v) g.add_vertex(v);
    // root can reach only 1,2
    g.add_edge(0,1,1.0);
    g.add_edge(1,2,1.0);
    // nodes 3,4 disconnected from root
    g.add_edge(3,4,1.0);

    auto arb = g.prims_algorithm(0);
    // Implementation returns {} when not all nodes reachable for arborescence
    // (based on "in == inf" check) => empty result
    CHECK(arb.empty());
}

// ============================== Section: SCC / CC ==============================

TEST_CASE("SCC: multiple strongly connected components with isolates") {
    Graph<int> g(0,true);
    for (int v=0; v<6; ++v) g.add_vertex(v);
    // SCC1: 0 <-> 1
    g.add_edge(0,1,1.0); g.add_edge(1,0,1.0);
    // SCC2: 2 -> 3 -> 2
    g.add_edge(2,3,1.0); g.add_edge(3,2,1.0);
    // SCC3: 4 alone; 5 alone
    auto comps = g.kosarajus_algorithm_scc();
    auto S = to_set_of_sets(comps);
    // Expect four components: {0,1}, {2,3}, {4}, {5}
    std::vector<std::set<int>> expected = { {0,1}, {2,3}, {4}, {5} };
    std::sort(expected.begin(), expected.end(),
              [](const auto& a, const auto& b){
                  if (a.size()!=b.size()) return a.size()<b.size();
                  return *a.begin()<*b.begin();
              });
    CHECK(S == expected);
}

TEST_CASE("Connected Components (undirected): two components") {
    Graph<int> g = make_empty_graph<int>(false);
    for (int v=0; v<5; ++v) g.add_vertex(v);
    // comp A: 0-1-2
    g.add_edge(0,1,1.0);
    g.add_edge(1,2,1.0);
    // comp B: 3-4
    g.add_edge(3,4,1.0);

    auto comps = g.kosarajus_algorithm_scc(); // dispatches to CC for undirected
    auto S = to_set_of_sets(comps);
    std::vector<std::set<int>> expected = { {0,1,2}, {3,4} };
    std::sort(expected.begin(), expected.end(),
              [](const auto& a, const auto& b){
                  if (a.size()!=b.size()) return a.size()<b.size();
                  return *a.begin()<*b.begin();
              });
    CHECK(S == expected);
}

// ============================== Section: Max-Flow (Edmonds–Karp) ==============================

TEST_CASE("Max-Flow: classic small network") {
    // s=0 -> 1 (10), 0 -> 2 (5), 1 -> 2 (15), 1 -> 3 (10), 2 -> 4 (10), 3 -> 4 (10)
    Graph<int> g(0,true);
    for (int v=0; v<=4; ++v) g.add_vertex(v);
    g.add_edge(0,1,10.0);
    g.add_edge(0,2,5.0);
    g.add_edge(1,2,15.0);
    g.add_edge(1,3,10.0);
    g.add_edge(2,4,10.0);
    g.add_edge(3,4,10.0);

    double f = g.edmon_karp_algorithm(0,4);
    CHECK(f == doctest::Approx(15.0));
}

TEST_CASE("Max-Flow: layered network sanity") {
    auto [g,s,t] = make_layered_flow<int>(3, 3, 7.0);
    double f = g.edmon_karp_algorithm(s,t);
    // cut is 3*7 => 21
    CHECK(f == doctest::Approx(21.0));
}

TEST_CASE("Max-Flow: zero-capacity edges => flow stays zero") {
    Graph<int> g(0,true);
    g.add_edge(0,1,0.0);
    g.add_edge(1,2,0.0);
    CHECK(g.edmon_karp_algorithm(0,2) == doctest::Approx(0.0));
}

TEST_CASE("Max-Flow: negative capacities are ignored by residual (treated as non-augmenting)") {
    Graph<int> g(0,true);
    g.add_edge(0,1,-5.0);
    g.add_edge(1,2,-3.0);
    CHECK(g.edmon_karp_algorithm(0,2) == doctest::Approx(0.0));
}

TEST_CASE("Max-Flow: source/sink not in graph => flow 0") {
    Graph<int> g(0,true);
    g.add_edge(1,2,5.0);
    CHECK(g.edmon_karp_algorithm(99,100) == doctest::Approx(0.0));
}

// ============================== Section: Hamiltonian Cycle ==============================

TEST_CASE("Hamilton: simple cycle exists (undirected 4-cycle)") {
    Graph<int> g = make_cycle_graph<int>(4,false,1.0);
    auto cyc = g.hamilton_cycle(0);
    CHECK(!cyc.empty());
    CHECK(cyc.front() == 0);
    CHECK(cyc.back() == 0);
    CHECK(cyc.size() == 5);
}

TEST_CASE("Hamilton: no cycle on a path graph") {
    Graph<int> g = make_path_graph<int>(5,false,1.0);
    auto cyc = g.hamilton_cycle(0);
    CHECK(cyc.empty());
}

TEST_CASE("Hamilton: start vertex not in graph => empty result") {
    Graph<int> g(0,false);
    g.add_edge(1,2,1.0);
    auto cyc = g.hamilton_cycle(42);
    CHECK(cyc.empty());
}

TEST_CASE("Hamilton: complete graph K_n always has Hamiltonian cycle for n>=3") {
    for (int n=3; n<=8; ++n) {
        auto g = make_complete_graph<int>(n, 1.0);
        auto cyc = g.hamilton_cycle(0);
        CHECK(!cyc.empty());
        CHECK(cyc.size() == static_cast<size_t>(n+1));
    }
}

// ============================== Section: String Output ==============================

TEST_CASE("to_string_with_weights: capacity label in directed graph") {
    Graph<int> g(0,true);
    g.add_edge(0,1,3.0);
    auto s1 = g.to_string_with_weights(true);
    CHECK(s1.find("cap=") != std::string::npos);
    auto s2 = g.to_string_with_weights(false);
    CHECK(s2.find("w=") != std::string::npos);
}

TEST_CASE("operator<< basic shape") {
    Graph<int> g(0,false);
    g.add_edge(0,1,2.5);
    std::ostringstream os;
    os << g;
    auto out = os.str();
    CHECK(out.find("{") != std::string::npos);
    CHECK(out.find("(") != std::string::npos);
}

// ============================== Section: Invalid Parameters & Edge Cases ==============================

TEST_CASE("Invalid: Prim on empty graph => empty result") {
    auto g = make_empty_graph<int>(false);
    auto mst = g.prims_algorithm(0);
    CHECK(mst.empty());
}

TEST_CASE("Invalid: Prim with root not present => empty result") {
    Graph<int> g(0,false);
    g.add_edge(1,2,3.0);
    auto mst = g.prims_algorithm(99);
    CHECK(mst.empty()); // priority queue starts with (99,99,0) but no adjacency => nothing added
}

TEST_CASE("Invalid: Arborescence with root not present => empty result") {
    Graph<int> g(0,true);
    g.add_edge(1,2,1.0);
    auto arb = g.prims_algorithm(99);
    CHECK(arb.empty());
}

TEST_CASE("Invalid: Euler circuit on non-Eulerian graph => empty") {
    Graph<int> g = make_path_graph<int>(4,false,1.0);
    CHECK(g.is_eulerian() == false);
    auto circ = g.euler_circuit();
    CHECK(circ.empty());
}

TEST_CASE("Invalid: Directed Euler with in/out mismatch => empty circuit") {
    Graph<int> g(0,true);
    g.add_edge(0,1,1.0);
    g.add_edge(1,2,1.0);
    g.add_edge(0,2,1.0); // in/out mismatch
    CHECK(g.is_eulerian() == false);
    auto circ = g.euler_circuit();
    CHECK(circ.empty());
}

TEST_CASE("Invalid: Max-Flow on disconnected s-t => zero") {
    Graph<int> g(0,true);
    g.add_edge(0,1,5.0);
    g.add_edge(2,3,5.0);
    CHECK(g.edmon_karp_algorithm(0,3) == doctest::Approx(0.0));
}

TEST_CASE("Invalid: Hamilton call on singleton/empty graphs") {
    auto g0 = make_empty_graph<int>(false);
    auto res0 = g0.hamilton_cycle(0);
    CHECK(res0.empty()); // implementation returns {} for non-existing start

    Graph<int> g1(0,false);
    g1.add_vertex(7);
    auto res1 = g1.hamilton_cycle(7);
    // A Hamiltonian cycle requires closing back to start; single-node with no self-edge will fail
    CHECK(res1.empty());
}

// ============================== Section: Consistency & Integration ==============================

TEST_CASE("Integration: MST over grid vs. number of edges") {
    auto g = make_grid_graph<int>(5,5,1.0);
    auto mst = g.prims_algorithm(0);
    // Grid of 25 nodes, MST must have 24 edges
    CHECK(mst.size() == 24);
}

TEST_CASE("Integration: SCC after turning undirected edges into directed both-ways") {
    Graph<int> g(0,true);
    // Create a 3-node undirected path effect by adding both directions
    g.add_edge(0,1,1.0); g.add_edge(1,0,1.0);
    g.add_edge(1,2,1.0); g.add_edge(2,1,1.0);
    auto comps = g.kosarajus_algorithm_scc();
    // All nodes mutually reachable via two-way chain? 0<->1 and 1<->2, but 0<->2 via 1 => strongly connected
    // In directed sense, 0->1->2 and 2->1->0 are possible => one SCC
    CHECK(comps.size() == 1);
    CHECK(comps.front().size() == 3);
}

TEST_CASE("Integration: compute Euler, then remove edge, recheck") {
    Graph<int> g = make_cycle_graph<int>(6,false,1.0);
    CHECK(g.is_eulerian() == true);
    auto circ = g.euler_circuit();
    CHECK(!circ.empty());
    g.remove_edge(0,1);
    CHECK(g.is_eulerian() == false);
}

TEST_CASE("Integration: run Max-Flow then check that output formatting still works") {
    auto [g,s,t] = make_layered_flow<int>(2, 4, 3.0);
    double f = g.edmon_karp_algorithm(s,t);
    CHECK(f == doctest::Approx(12.0));
    auto s1 = g.to_string_with_weights(true);
    CHECK(s1.find("{") != std::string::npos);
}

// ============================== Section: Stress / Performance ==============================

#if HEAVY_TESTS && ENABLE_PERF_TESTS

TEST_CASE("Perf: Connected Components on large undirected sparse graph") {
    const int N = SZ(9000);
    const double p = 3.0 / N;
    auto g = make_random_undirected<int>(N, p, 2025);
    auto t0 = std::chrono::steady_clock::now();
    auto comps = g.kosarajus_algorithm_scc(); // CC path for undirected
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("N=" << N << " p=" << p << " ms=" << ms << " limit=" << PERF_MS_LIMIT);
    CHECK(ms < PERF_MS_LIMIT);
    CHECK(!comps.empty());
}

TEST_CASE("Perf: Euler circuit on large even-degree ring") {
    const int N = SZ(25000);
    Graph<int> g(0,false);
    for (int i=0;i<N;i++) g.add_vertex(i);
    for (int i=0;i<N;i++) {
        int j = (i+1)%N;
        g.add_edge(i,j,1.0);
    }
    auto t0 = std::chrono::steady_clock::now();
    CHECK(g.is_eulerian() == true);
    auto circuit = g.euler_circuit();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("N=" << N << " ms=" << ms << " limit=" << PERF_MS_LIMIT << " circuit_size=" << circuit.size());
    CHECK(ms < PERF_MS_LIMIT);
    CHECK(circuit.size() == static_cast<size_t>(N+1));
}

TEST_CASE("Perf: SCC on large directed graph") {
    const int N = SZ(12000);
    const double p = 4.0 / N;
    auto g = make_random_directed<int>(N, p, 7);
    auto t0 = std::chrono::steady_clock::now();
    auto comps = g.kosarajus_algorithm_scc();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("N=" << N << " p=" << p << " ms=" << ms << " limit=" << PERF_MS_LIMIT << " num_comps=" << comps.size());
    CHECK(ms < PERF_MS_LIMIT);
    CHECK(!comps.empty());
}

TEST_CASE("Perf: Max-Flow on layered network") {
    const int layers = SZ(6);
    const int L = SZ(30);
    auto [g,s,t] = make_layered_flow<int>(layers, L, 5.0);
    auto t0 = std::chrono::steady_clock::now();
    double f = g.edmon_karp_algorithm(s,t);
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("layers=" << layers << " L=" << L << " f=" << f << " ms=" << ms << " limit=" << PERF_MS_LIMIT);
    CHECK(f == doctest::Approx(L*5.0));
    CHECK(ms < PERF_MS_LIMIT);
}

TEST_CASE("Perf: Hamilton attempt on dense complete graph (small due to exponential)") {
    // Hamilton is exponential; keep size modest but still non-trivial.
    const int n = 12; // do not scale blindly
    auto g = make_complete_graph<int>(n, 1.0);
    auto t0 = std::chrono::steady_clock::now();
    auto cyc = g.hamilton_cycle(0);
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("n=" << n << " ms=" << ms << " limit=" << PERF_MS_LIMIT);
    CHECK(!cyc.empty());
    CHECK(cyc.size() == static_cast<size_t>(n+1));
    CHECK(ms < PERF_MS_LIMIT);
}

#endif // HEAVY_TESTS && ENABLE_PERF_TESTS

#if VERY_HEAVY_TESTS && ENABLE_PERF_TESTS
// These tests are intentionally extreme. Enable with -DVERY_HEAVY_TESTS=1 if your machine can handle it.

TEST_CASE("Very-Heavy: CC on much larger sparse graph") {
    const int N = SZ(40000);
    const double p = 3.0 / N;
    auto g = make_random_undirected<int>(N, p, 909090);
    auto t0 = std::chrono::steady_clock::now();
    auto comps = g.kosarajus_algorithm_scc();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("N=" << N << " p=" << p << " ms=" << ms);
    CHECK(!comps.empty());
}

TEST_CASE("Very-Heavy: SCC on larger directed graph") {
    const int N = SZ(40000);
    const double p = 4.0 / N;
    auto g = make_random_directed<int>(N, p, 123456);
    auto t0 = std::chrono::steady_clock::now();
    auto comps = g.kosarajus_algorithm_scc();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("N=" << N << " p=" << p << " ms=" << ms << " num_comps=" << comps.size());
    CHECK(!comps.empty());
}

#endif // VERY_HEAVY_TESTS && ENABLE_PERF_TESTS

// ============================== Section: Additional Edge/Weird Cases ==============================

TEST_CASE("Weird: mixing negative and positive weights in undirected graph for Prim") {
    Graph<int> g(0,false);
    for (int i=0;i<6;i++) g.add_vertex(i);
    g.add_edge(0,1,-2.0);
    g.add_edge(1,2, 3.0);
    g.add_edge(2,3,-1.0);
    g.add_edge(3,4, 2.5);
    g.add_edge(4,5, 1.0);
    g.add_edge(0,5, 5.0);
    auto mst = g.prims_algorithm(0);
    CHECK(mst.size() == 5);
    double sum=0; for (auto& e : mst) sum += e.edge_weight;
    CHECK(sum < 10.0);
}

TEST_CASE("Weird: directed graph with self-loops and dead-ends for SCC") {
    Graph<int> g(0,true);
    for (int i=0;i<6;i++) g.add_vertex(i);
    g.add_edge(0,0,1.0); // self-loop SCC of {0}
    g.add_edge(1,2,1.0);
    g.add_edge(2,1,1.0); // SCC {1,2}
    g.add_edge(3,4,1.0); // 3->4 only (no return), separate singletons unless 4->3
    g.add_edge(5,5,1.0); // self-loop SCC {5}
    auto comps = g.kosarajus_algorithm_scc();
    // Expect SCCs: {0}, {1,2}, {3}, {4}, {5}
    auto S = to_set_of_sets(comps);
    std::vector<std::set<int>> expected = { {0}, {1,2}, {3}, {4}, {5} };
    std::sort(expected.begin(), expected.end(),
              [](const auto& a, const auto& b){
                  if (a.size()!=b.size()) return a.size()<b.size();
                  return *a.begin()<*b.begin();
              });
    CHECK(S == expected);
}

TEST_CASE("Weird: flow network with parallel logical paths but shared bottleneck") {
    // s=0; two mid nodes 1,2 both feeding into t=3 but with narrow edges to t
    Graph<int> g(0,true);
    for (int v=0; v<=3; ++v) g.add_vertex(v);
    g.add_edge(0,1,100.0);
    g.add_edge(0,2,100.0);
    g.add_edge(1,3,1.0);
    g.add_edge(2,3,1.0);
    double f = g.edmon_karp_algorithm(0,3);
    CHECK(f == doctest::Approx(2.0));
}

TEST_CASE("Weird: Hamilton on complete graph missing one edge (still Hamiltonian)") {
    Graph<int> g(0,false);
    for (int i=0;i<5;i++) g.add_vertex(i);
    // complete
    for (int i=0;i<5;i++)
        for (int j=i+1;j<5;j++)
            g.add_edge(i,j,1.0);
    // remove a single edge (not breaking Hamiltonicity)
    g.remove_edge(0,3);
    auto cyc = g.hamilton_cycle(0);
    CHECK(!cyc.empty());
    CHECK(cyc.size() == 6);
}

// ============================== Section: Robustness of API Behavior ==============================

TEST_CASE("Robustness: to_string_with_weights on empty/directed/undirected") {
    auto g0 = make_empty_graph<int>(false);
    auto s0 = g0.to_string_with_weights(false);
    CHECK(s0.find("{") != std::string::npos);

    auto g1 = make_empty_graph<int>(true);
    auto s1 = g1.to_string_with_weights(true);
    CHECK(s1.find("{") != std::string::npos);

    Graph<int> g2(0,true);
    g2.add_edge(10,11,2.0);
    auto s2 = g2.to_string_with_weights(true);
    CHECK(s2.find("cap=") != std::string::npos);
}

TEST_CASE("Robustness: operator<< prints neighbors consistently") {
    Graph<int> g(0,false);
    g.add_edge(0,1,2.0);
    g.add_edge(0,2,3.0);
    std::ostringstream os;
    os << g;
    auto txt = os.str();
    CHECK(txt.find("(1, 2") != std::string::npos);
    CHECK(txt.find("(2, 3") != std::string::npos);
}

TEST_CASE("Robustness: toggling directed flag changes only algorithmic dispatch, not adjacency layout") {
    Graph<int> g(0,false);
    g.add_edge(0,1,1.0);
    CHECK(g.is_directed() == false);
    // The adjacency remains as was (undirected edges were added both-ways)
    CHECK(g.out_degree(0) == 1);
    CHECK(g.out_degree(1) == 1);
}

// ============================== Section: Additional Performance Micro-Benchmarks ==============================

#if HEAVY_TESTS && ENABLE_PERF_TESTS

TEST_CASE("Perf-Micro: to_string_with_weights on medium graph") {
    auto g = make_random_undirected<int>(SZ(2000), 6.0/SZ(2000), 4242, 1.0, 2.0);
    auto t0 = std::chrono::steady_clock::now();
    auto s = g.to_string_with_weights(false);
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("s.size()=" << s.size() << " ms=" << ms);
    CHECK(ms < PERF_MS_LIMIT);
}

TEST_CASE("Perf-Micro: build and query degrees") {
    int N = SZ(10000);
    double p = 4.0 / N;
    auto g = make_random_directed<int>(N, p, 2026, 1.0, 1.0);
    auto t0 = std::chrono::steady_clock::now();
    size_t sum_in=0, sum_out=0;
    for (int i=0;i<N;i++) {
        sum_in += g.in_degree(i);
        sum_out += g.out_degree(i);
    }
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("sum_in=" << sum_in << " sum_out=" << sum_out << " ms=" << ms);
    CHECK(sum_in == sum_out);
    CHECK(ms < PERF_MS_LIMIT);
}

#endif // HEAVY_TESTS && ENABLE_PERF_TESTS
