#pragma once
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <stack>
#include <memory>
namespace Graph_implementation{

    //==== custom hash for std::pair ====
    // Needed because unordered_set doesn't know how to hash std::pair by default.
    // This combines the hashes of the pair's first and second elements.

struct pair_hash {
        template <typename A, typename B>
        std::size_t operator()(const std::pair<A,B>& pair) const{
            return std::hash<A>()(pair.first) ^ (std::hash<B>()(pair.second) << 1);
        };
};

template <typename T>
class Graph{

   private:
   size_t vertices_amount;
   std::unordered_map<T, std::unordered_set<std::pair<T, double>, pair_hash>> graph;
   
   
   public:
   Graph(size_t amount):vertices_amount(amount){};
   ~Graph() = default;
   Graph(const Graph &other) = delete;
   Graph operator=(Graph &other) = delete;
   Graph(Graph &&other) = delete;
   Graph operator=(Graph &&other) = delete;


   void add_vertex(const T &vertex){
    graph[vertex] = {};
   }

   void add_edge(const T &vertex_v, const T &vertex_u,double weight){
    graph[vertex_v].insert({vertex_u,weight});
    graph[vertex_u].insert({vertex_v,weight});
   }

   void remove_edge(const T &vertex_v, const T &vertex_u,double weight){
    graph[vertex_v].erase({vertex_u,weight});
    graph[vertex_u].erase({vertex_v,weight});
   }



bool is_eulerian() const{
    return all_even_degree() && is_connected();
}


 size_t degree(const T &vertex_v) const{
    auto it = graph.find(vertex_v);
    return it != graph.end() ? it->second.size() : 0;
}


bool all_even_degree() const{
    for(const auto&[vertex,neighbors]: graph){
        if(degree(vertex) % 2 != 0){
            return false;
        }
    }
    return true;
}


bool is_connected() const{
    if( graph.empty()) return true;

    std::stack<T> st;
    std::unordered_set<T> visited;

    T start = graph.begin()->first;
    st.push(start);


    while(!st.empty()){

        T vertex_u = st.top();
        st.pop();
        if(visited.count(vertex_u)) continue;
        visited.insert(vertex_u);

        auto it = graph.find(vertex_u);
        for(const auto&[neighbor,weight] : it->second){
            if(!visited.count(neighbor)){
                st.push(neighbor);
            }
        }
    }

    return visited.size() == graph.size();
    
}


std::unique_ptr<std::vector<T>> find_euler_circut() {
    if (!is_eulerian()) {
        throw std::runtime_error("Graph is Not Eulerian.");
    }

    std::stack<T> st;
    T start = graph.begin()->first;
    std::unique_ptr<std::vector<T>> circuit_list = std::make_unique<std::vector<T>>();

    auto temp_graph = this->graph;

    st.push(start);

    while (!st.empty()) {
        T vertex_u = st.top();

        if (!temp_graph[vertex_u].empty()) {
            auto neighbor_it = temp_graph[vertex_u].begin();
            T neighbor = neighbor_it->first;
            double weight = neighbor_it->second;

            temp_graph[vertex_u].erase(neighbor_it);
            temp_graph[neighbor].erase({vertex_u, weight});

            st.push(neighbor);
        } else {
            st.pop();
            circuit_list->emplace_back(vertex_u);
        }
    }

    return std::move(circuit_list);
}

   template<typename U>
    friend std::ostream& operator<<(std::ostream&, const Graph<U>&);

};


template <typename T>
std::ostream &operator<<(std::ostream &os, const Graph<T> &other) {
    os << "{\n";
    for(const auto& [vertex,neighbors] : other.graph){
       os << " " << vertex << " : [ ";
       for(const auto& [adjacent,weight] : neighbors){
        os << "(" << adjacent << ", " << weight << ") ";
       }
       os <<"]\n";
    }
    os << "}";
    return os;
}



};