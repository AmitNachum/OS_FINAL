#pragma once
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <stack>
#include <memory>
#include <functional>




template <typename K> 
struct Edge{
    K vertex_w;
    K vertex_r;
    double edge_weight;

    Edge(K vertex1,K vertex2,double weight_value):vertex_w(vertex1),vertex_r(vertex2),edge_weight(weight_value){}
    ~Edge() = default;

    bool operator>(const Edge& other) const{
        return edge_weight > other.edge_weight;
    }
};

template <typename J>
struct Vertex_weight{
    J vertex_u;
    double weight_heap;

    Vertex_weight(J vertex,double weight_i):vertex_u(vertex),weight_heap(weight_i){}
    ~Vertex_weight() = default;

    bool operator>(const Vertex_weight& other) const{
        return this->weight_heap > other.weight_heap;
    }
};


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
   Graph(size_t amount):vertices_amount(amount){}
   ~Graph() = default;
   Graph(const Graph &other) = delete;
   Graph operator=(Graph &other) = delete;
   Graph(Graph &&other) = delete;
   Graph operator=(Graph &&other) = delete;


   void add_vertex(const T &vertex){
    graph[vertex] = {};
   }




   void add_edge(const T &vertex_v, const T &vertex_u,double weight,bool directed = false){
    auto& neighbor_set = graph[vertex_v];

    if(!directed){
    auto it = std::find_if(neighbor_set.begin(),neighbor_set.end(), 
                     [&](auto const &pr){
                        return pr.first == vertex_u;
                                    });

    if(it == neighbor_set.end()){
    graph[vertex_v].insert({vertex_u,weight});
    graph[vertex_u].insert({vertex_v,weight});
   } 
} else{

    auto it = std::find_if(neighbor_set.begin(),neighbor_set.end(),
    [&](auto const &pair){
        return pair.first == vertex_u;
    });

    if(it == neighbor_set.end()){
        graph[vertex_v].insert({vertex_u,weight});
    }

}  
}

   void remove_edge(const T &vertex_v, const T &vertex_u,double weight,bool directed = false){
    if(!directed){
    graph[vertex_v].erase({vertex_u,weight});
    graph[vertex_u].erase({vertex_v,weight});
   }
   else
     graph[vertex_v].erase({vertex_u,weight});
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



//Prim's Algorithm
std::vector<struct Edge<T>> prims_algorithm(const T& source){
    auto &adj_map = this->graph;
    std::priority_queue<Edge<T>,std::vector<Edge<T>>,std::greater<Edge<T>>> pq;

    std::unordered_set<T> inMST;
    std::vector<struct Edge<T>> result;
    
    pq.push({source,source,0}); //Dummy node


    while(!pq.empty()){

        auto [v,u,w] = pq.top();
        pq.pop();

        if(inMST.count(v) > 0) continue;

        if(v != u){ // skip dummy
            result.emplace_back(v,u,w);
        }
        inMST.insert(v);

        for(auto& [neighbor,weight]: adj_map[v]){
            if(inMST.count(neighbor) == 0)
                pq.push({v,neighbor,weight});
        }
    }

    return result;

}


using adj_list = std::unordered_map<T,std::unordered_set<std::pair<T,double>,pair_hash>>;

adj_list transpose_graph(bool directed = false){
    if(!directed){
        std::cout <<"Cannot transpose an undirected Graph" << std::endl;
        return {};  
    }

    adj_list reversed;


    for(const auto& [vertex,neighbors] : graph){
        reversed[vertex] = {};
    }

    
    for(const auto& [vertex,values] : graph){
        for(auto& [neighbor,weight] : values){
            reversed[neighbor].insert({vertex,weight});
        }
    }
    

    return reversed;


}


private:
    void first_dfs(const T& vertex,std::stack<T>& stack_scc){
        std::stack<std::pair<T,bool>> dfs_stack;
        std::unordered_set<T> visited;

        dfs_stack.push({vertex,false});

        while(!dfs_stack.empty()){
            auto [u,backtrack] = dfs_stack.top();
            dfs_stack.pop();

            if(backtrack){
            stack_scc.push(u);
            continue;
            }

            if(visited.count(u)) continue;
            visited.insert(u);

            dfs_stack.push({u,true});//simulating Post order
            
            for(const auto& [neighbor,weight] : graph[u]){
                if(visited.count(neighbor) == 0){
                    dfs_stack.push({neighbor,false});
                }
            }
            
        }

    }

    void second_dfs(const T& vertex, const adj_list& transposed_g, std::unordered_set<T>& visited, std::vector<T>& current_scc){

        std::stack<T> dfs_2_stack;
        dfs_2_stack.push(vertex);//dummy node;
        visited.insert(vertex);
        
        
        while(!dfs_2_stack.empty()){
            auto u = dfs_2_stack.top();
            dfs_2_stack.pop();
            current_scc.push_back(u);


            for(const auto& [neighbor,weight] : transposed_g.at(u)){
                if(visited.count(neighbor) == 0){
                    dfs_2_stack.push(neighbor);
                    visited.insert(neighbor);
                }
            }

        }
    }



public:

std::vector<std::vector<T>> kosarajus_algorithm_scc(){
    std::stack<T> scc_stack;
    std::unordered_set<T> visited;

    for(const auto& [vertex,neighbors]: graph){
        if(visited.count(vertex) == 0){
        first_dfs(vertex,scc_stack);
    }
 }

    adj_list reversed = transpose_graph(true);

    


    visited.clear();
    std::vector<std::vector<T>> result;


    while(!scc_stack.empty()){
        auto v = scc_stack.top();
        scc_stack.pop();

        if(visited.count(v) == 0){
            std::vector<T> component;
            second_dfs(v,reversed,visited,component);
            result.emplace_back(component);
        }
    }
    return result;
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