#pragma once
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <stack>
#include <memory>
#include <functional>
#include <limits>





template <typename K> 
struct Edge {
    K vertex_w;
    K vertex_r;
    double edge_weight = 0.0;
    double capacity = 0.0;
    double current_flow = 0.0;

    Edge(K vertex1, K vertex2, double weight_value)
        : vertex_w(vertex1), vertex_r(vertex2), edge_weight(weight_value) {}

    Edge(K vertex1, K vertex2, double capacity_val, double current_flow_val = 0.0)
        : vertex_w(vertex1), vertex_r(vertex2),
          capacity(capacity_val), current_flow(current_flow_val) {}

    ~Edge() = default;   
    Edge() = default;      

    void set_current_flow(double value) {
        current_flow = value;
    }

    double residual_capacity() const {
        return capacity - current_flow;
    }

    bool operator>(const Edge& other) const {
        return edge_weight > other.edge_weight;
    }


bool operator==(const Edge<K>& rhs) {
    return vertex_w == rhs.vertex_w &&
           vertex_r == rhs.vertex_r;
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


/*===custom hash for Edge struct===*/

template<typename K>
struct edge_hash {
    std::size_t operator()(const Edge<K>& edge) const {
        std::size_t hash1 = std::hash<K>()(edge.vertex_w);
        std::size_t hash2 = std::hash<K>()(edge.vertex_r);
        return hash1 ^ (hash2 << 1); // Combine hashes
    }
};




template <typename T>
class Graph{

   private:
   size_t vertices_amount;
   std::unordered_map<T, std::unordered_set<std::pair<T, double>, pair_hash>> graph;
   const T& start_vertex;

   
   
   public:
   Graph(size_t amount):vertices_amount(amount){}
   ~Graph() = default;
   Graph(const Graph &other) = delete;
   Graph operator=(Graph &other) = delete;
   Graph(Graph &&other) = delete;
   Graph operator=(Graph &&other) = delete;


   void add_vertex(const T &vertex){
    if(graph.empty()){
        start_vertex = vertex;
    }
    graph[vertex] = {};
   }

   T& get_first(){
       return start_vertex;
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
        nullptr;
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
           Request(const std::string& name,std::vector<std::string>& args):name(name),args(args){

  } }
            
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


using residual_graph = std::unordered_map<T,std::vector<Edge<T>>>;
residual_graph convert_to_residual() {
    residual_graph rs;

    // Pass 1: Add all forward edges from the original graph.
    // This correctly sets their initial capacities.
    for (const auto& [u, neighbors] : graph) {
        for (const auto& [v, cap] : neighbors) {
            rs[u].emplace_back(u, v, cap, 0.0);
        }
    }

    // Pass 2: Add backward edges, but ONLY if an edge in that direction
    // doesn't already exist from an edge in the original graph.
    for (const auto& [u, neighbors] : graph) {
        for (const auto& [v, cap] : neighbors) {
            // Check if the reverse edge v -> u already exists in our new graph `rs`.
            bool reverse_exists = false;
            if (rs.count(v)) {
                for (const auto& edge : rs.at(v)) {
                    if (edge.vertex_r == u) {
                        reverse_exists = true;
                        break;
                    }
                }
            }
            // If it doesn't exist, add it with 0 capacity.
            if (!reverse_exists) {
                rs[v].emplace_back(v, u, 0.0, 0.0);
            }
        }
    }
    return rs;
}



bool bfs_source_to_sink(
    const T& source,
    const T& sink,
    const residual_graph& rs,
    std::unordered_map<T, T>& parent
) {                    
    std::unordered_set<T> visited;
    std::queue<T> que;

    visited.insert(source);
    que.push(source);

    std::cout << "BFS from " << source << " to " << sink << "\n";

    while (!que.empty()) {
        T u = que.front(); que.pop();

        for (const Edge<T>& e : rs.at(u)) {
            T v = e.vertex_r;
            double rc = e.residual_capacity();
            if (rc > 0 && !visited.count(v)) {
                parent[v] = u;
                std::cout << "    visiting, parent[" << v << "]=" << u << "\n";
                if (v == sink) {
                    std::cout << "    reached sink!\n";
                    return true;
                }
                visited.insert(v);
                que.push(v);
            }
        }
    }

    std::cout << "  no path found\n";
    return false;
}


    bool check_for_edge(const T& vertex_1,const T& vertex_2)const{

        auto it = graph.find(vertex_1);
        if(it == graph.end()){
            return false;
        }
        //it is an iterator of a pair (neighbor,weight)

        return std::any_of(it->second.begin(),it->second.end(),
        [&](const auto& pair){
            return pair.first == vertex_2;
        });
    }

bool dfs_for_hamilton_cycle(const T& vertex,
    const T& start,
    std::vector<T>& path,
    std::unordered_set<T>& visited){
    
        if(path.size() == vertices_amount){

            if(check_for_edge(vertex,start)){
                path.push_back(start);
                return true;

            }
            return false;
        }

        visited.insert(vertex);



        for(const auto& [neighbor,weight]: graph[vertex]){
           
            if(!visited.count(neighbor)){
                path.push_back(neighbor);
                if(dfs_for_hamilton_cycle(neighbor,start,path,visited)){
                    return true;
                }
                path.pop_back();
                visited.erase(neighbor); 

            }
        }

        visited.erase(vertex);
        return false;


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




double edmon_karp_algorithm(const T& source, const T& sink){
    residual_graph rs = convert_to_residual();
    double flow_answer = 0.0;


    std::unordered_map<T,T> parent;



    while(bfs_source_to_sink(source,sink,rs,parent)){
        std::cout <<"found an augmented path \n";
        double path_flow = std::numeric_limits<double>::infinity(); // starting with a max infinity
        T curr = sink;


        //finding the bottle neck
        // finding the bottleneck (only positive‐capacity edges)
        while (curr != source) {
            T prev = parent[curr];
                for (const Edge<T>& edge : rs[prev]) {
                    if (edge.vertex_r == curr) {
                        path_flow = std::min(path_flow, edge.residual_capacity());
                        break;
                }
            }
        curr = prev;
    }

    


        curr = sink;
        //update the residual graph
        while(curr != source){
            auto prev = parent[curr];

            //Forward edge (prev -> curr)
            for (auto &e : rs[prev]) {
                if (e.vertex_r==curr) { e.current_flow += path_flow; break; }
            }


            //Backwards edge (curr -> prev)
            for (auto &e : rs[curr]) {
                if (e.vertex_r==prev) { e.current_flow -= path_flow; break; }
            }


            curr = prev;

        }   
        flow_answer += path_flow;


     parent.clear();
        

    }

    return flow_answer;
}




 const std::vector<T>& hamilton_cycle(const T& start){
    std::vector<T> cycle;
    std::unordered_set<T> visited;
    cycle.reserve(vertices_amount + 1);

    cycle.push_back(start);
    visited.insert(start);

    if(dfs_for_hamilton_cycle(start,start,cycle,visited)){
        return cycle;
    } else{
        return {};
    }
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