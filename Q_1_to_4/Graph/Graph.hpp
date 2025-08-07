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
#include <sstream>
#include <vector>
#include <set>

template <typename K> 
struct Edge {
    K vertex_w;
    K vertex_r;
    double edge_weight = 0.0;
    double capacity = 0.0;
    double current_flow = 0.0;

    Edge(K v1, K v2, double w) : vertex_w(v1), vertex_r(v2), edge_weight(w) {}
    Edge(K v1, K v2, double cap, double flow, int /*tag*/)
        : vertex_w(v1), vertex_r(v2), capacity(cap), current_flow(flow) {}
    // helper ctor for residual usage (cap,flow)
    Edge(K v1, K v2, double cap, double flow) : Edge(v1, v2, cap, flow, 0) {}

    ~Edge() = default;   
    Edge() = default;      

    void set_current_flow(double value) { current_flow = value; }
    double residual_capacity() const { return capacity - current_flow; }
    bool operator>(const Edge& other) const { return edge_weight > other.edge_weight; }

    bool operator==(const Edge<K>& rhs) const {
        return vertex_w == rhs.vertex_w && vertex_r == rhs.vertex_r;
    }
};

namespace Graph_implementation{

struct pair_hash {
    template <typename A, typename B>
    std::size_t operator()(const std::pair<A,B>& pair) const{
        return std::hash<A>()(pair.first) ^ (std::hash<B>()(pair.second) << 1);
    };
};

template<typename K>
struct edge_hash {
    std::size_t operator()(const Edge<K>& edge) const {
        std::size_t h1 = std::hash<K>()(edge.vertex_w);
        std::size_t h2 = std::hash<K>()(edge.vertex_r);
        return h1 ^ (h2 << 1);
    }
};

template <typename T>
class Graph{
   private:
    size_t vertices_amount;
    std::unordered_map<T, std::unordered_set<std::pair<T, double>, pair_hash>> graph;
    T start_vertex{};
    bool directed_{false}; // <--- global graph mode

   public:
    // directed has default false to keep backward compatibility
    Graph(size_t amount, bool directed=false)
        : vertices_amount(amount), directed_(directed) {}
    ~Graph() = default;
    Graph(const Graph &other) = delete;
    Graph& operator=(const Graph &other) = delete;
    Graph(Graph &&other) = delete;
    Graph& operator=(Graph &&other) = delete;

    bool is_directed() const { return directed_; }
    void set_directed(bool d) { directed_ = d; }

    void add_vertex(const T &vertex){
        if(graph.empty()){
            start_vertex = vertex;
        }
        graph[vertex] = {};
    }

    T& get_first(){ return start_vertex; }

    // Adds an edge using the graph's directedness flag.
    void add_edge(const T &u, const T &v, double w, bool /*ignored*/ = false){
        // NOTE: external 'directed' param is ignored; we use directed_ consistently.
        auto& nbrs = graph[u];

        if(!directed_){
            auto it = std::find_if(nbrs.begin(), nbrs.end(),
                [&](auto const &pr){ return pr.first == v; });
            if(it == nbrs.end()){
                graph[u].insert({v,w});
                graph[v].insert({u,w});
            }
        } else {
            auto it = std::find_if(nbrs.begin(), nbrs.end(),
                [&](auto const &pr){ return pr.first == v; });
            if(it == nbrs.end()){
                graph[u].insert({v,w});
            }
        }
    }

    void remove_edge(const T &u, const T &v, double w, bool /*ignored*/ = false){
        if(!directed_){
            graph[u].erase({v,w});
            graph[v].erase({u,w});
        } else {
            graph[u].erase({v,w});
        }
    }

    // ======================= Common helpers =======================
    size_t degree(const T &v) const{
        auto it = graph.find(v);
        return it != graph.end() ? it->second.size() : 0;
    }

    size_t out_degree(const T& v) const {
        auto it = graph.find(v);
        return (it == graph.end()) ? 0 : it->second.size();
    }

    size_t in_degree(const T& v) const {
        size_t deg = 0;
        for (const auto& [u, nbrs] : graph) {
            for (const auto& pr : nbrs) {
                if (pr.first == v) { ++deg; break; }
            }
        }
        return deg;
    }

    bool all_even_degree() const{
        for(const auto&[v,_]: graph){
            if(degree(v) % 2 != 0) return false;
        }
        return true;
    }

    // Undirected connectivity
    bool is_connected_undirected() const{
        if (graph.empty()) return true;
        std::stack<T> st;
        std::unordered_set<T> vis;
        T start = graph.begin()->first;
        st.push(start);
        while(!st.empty()){
            T u = st.top(); st.pop();
            if (!vis.insert(u).second) continue;
            for(const auto&[w,_] : graph.at(u)) if(!vis.count(w)) st.push(w);
        }
        return vis.size() == graph.size();
    }

    // Weak connectivity for directed graphs: ignore directions over non-zero-degree subgraph
    bool weakly_connected_nonzero() const {
        std::unordered_map<T, std::vector<T>> und;
        std::unordered_set<T> nonzero;
        for (const auto& [u, nbrs] : graph) {
            if (!nbrs.empty()) nonzero.insert(u);
            for (const auto& [v, _] : nbrs) {
                und[u].push_back(v);
                und[v].push_back(u);
                nonzero.insert(v);
            }
        }
        if (nonzero.empty()) return true; // trivial
        std::stack<T> st;
        std::unordered_set<T> vis;
        T start = *nonzero.begin();
        st.push(start);
        while (!st.empty()) {
            T u = st.top(); st.pop();
            if (vis.insert(u).second) {
                for (auto& w : und[u]) if (!vis.count(w)) st.push(w);
            }
        }
        for (auto& v : nonzero) if (!vis.count(v)) return false;
        return true;
    }

    // ======================= Euler =======================
    // Public facade: choose variant based on directed_
    bool is_eulerian() const {
        return directed_ ? is_eulerian_directed_impl() : is_eulerian_undirected_impl();
    }

    std::vector<T> euler_circuit() const {
        return directed_ ? euler_directed_impl() : euler_undirected_impl();
    }

   private:
    bool is_eulerian_undirected_impl() const {
        return all_even_degree() && is_connected_undirected();
    }

    bool is_eulerian_directed_impl() const {
        if (!weakly_connected_nonzero()) return false;
        for (const auto& [v, _] : graph) {
            size_t in = in_degree(v), out = out_degree(v);
            if ((in + out) > 0 && in != out) return false;
        }
        return true;
    }

    std::vector<T> euler_undirected_impl() const {
        if (!is_eulerian_undirected_impl()) return {};
        // Hierholzer on multiset emulation using copies
        std::unordered_map<T, std::vector<T>> adj;
        for (const auto& [u, s] : graph) {
            for (const auto& [v,_w]: s) adj[u].push_back(v);
        }
        // remove back-edges as we go using a multiset-like approach
        std::unordered_map<T, std::multiset<T>> ms;
        for (auto& [u, vs] : adj) for (auto v: vs) ms[u].insert(v);

        auto erase_und = [&](const T& a, const T& b){
            auto it = ms[a].find(b);
            if (it!=ms[a].end()) ms[a].erase(it);
            it = ms[b].find(a);
            if (it!=ms[b].end()) ms[b].erase(it);
        };

        T start = graph.begin()->first;
        for (const auto& [u,s] : graph) if (!s.empty()) { start=u; break; }

        std::stack<T> st; std::vector<T> circ;
        st.push(start);
        while(!st.empty()){
            T u = st.top();
            if (!ms[u].empty()){
                T v = *ms[u].begin();
                erase_und(u,v);
                st.push(v);
            } else {
                circ.push_back(u);
                st.pop();
            }
        }
        std::reverse(circ.begin(), circ.end());
        return circ;
    }

    std::vector<T> euler_directed_impl() const {
        if (!is_eulerian_directed_impl()) return {};
        std::unordered_map<T, std::vector<T>> adj; adj.reserve(graph.size());
        for (const auto& [u, nbrs] : graph) {
            auto& vec = adj[u];
            vec.reserve(nbrs.size());
            for (const auto& [v, _w] : nbrs) vec.push_back(v);
        }
        std::unordered_map<T, size_t> idx;
        std::stack<T> st;
        std::vector<T> circuit;

        T start = graph.empty() ? T{} : graph.begin()->first;
        for (const auto& [u, nbrs] : graph) if (!nbrs.empty()) { start = u; break; }

        st.push(start);
        while (!st.empty()) {
            T u = st.top();
            auto& vec = adj[u];
            size_t& i = idx[u];
            if (i < vec.size()) st.push(vec[i++]);
            else { circuit.push_back(u); st.pop(); }
        }
        std::reverse(circuit.begin(), circuit.end());
        return circuit;
    }

   public:
    // ======================= MST / Arborescence =======================
    // Public facade: same API name, internal dispatch.
    std::vector<Edge<T>> prims_algorithm(const T& root){
        return directed_ ? directed_arborescence_impl(root)
                         : prim_undirected_impl(root);
    }

   private:
    std::vector<Edge<T>> prim_undirected_impl(const T& source){
        auto &adj_map = this->graph;
        std::priority_queue<Edge<T>,std::vector<Edge<T>>,std::greater<Edge<T>>> pq;
        std::unordered_set<T> inMST;
        std::vector<Edge<T>> result;

        pq.push(Edge<T>(source,source,0.0)); // dummy

        while(!pq.empty()){
            Edge<T> top = pq.top(); pq.pop();
            auto v = top.vertex_r;
            auto u = top.vertex_w;
            auto w = top.edge_weight;

            if(inMST.count(v)) continue;

            if(v != u) result.emplace_back(u, v, w); // store as (u->v, w)
            inMST.insert(v);

            for(auto& [nbr,wt]: adj_map[v]){
                if(!inMST.count(nbr)) pq.push(Edge<T>(v,nbr,wt));
            }
        }
        return result;
    }

    // Chu–Liu/Edmonds (simplified reconstruction)
    std::vector<Edge<T>> directed_arborescence_impl(const T& root) {
        // Map nodes to indices
        std::vector<T> nodes; nodes.reserve(graph.size());
        std::unordered_map<T,int> id;
        int idx=0;
        for (auto& kv : graph) { id[kv.first]=idx++; nodes.push_back(kv.first); }
        if (id.find(root)==id.end()) return {};

        struct E { int u,v; double w; };
        std::vector<E> edges;
        for (auto& [u, nbrs] : graph) {
            int iu = id[u];
            for (auto& [v, w] : nbrs) {
                int iv = id[v];
                if (iu!=iv) edges.push_back({iu,iv,w});
            }
        }

        int N = (int)nodes.size();
        if (N==0) return {};
        int root_idx = id[root];

        double res = 0;
        std::vector<int> pre, idc, vis;
        std::vector<double> in;
        int n = N;
        std::vector<E> es = edges;

        auto get_weight_from_graph = [&](const T& a, const T& b)->double {
            auto it = graph.find(a);
            if (it != graph.end()) {
                for (const auto& pr : it->second) {
                    if (pr.first == b) return pr.second;
                }
            }
            return 0.0; // fallback
        };

        while (true) {
            in.assign(n, std::numeric_limits<double>::infinity());
            pre.assign(n, -1);

            for (auto &e : es) {
                if (e.u != e.v && e.w < in[e.v]) {
                    in[e.v] = e.w;
                    pre[e.v] = e.u;
                }
            }
            in[root_idx] = 0;

            for (int i=0;i<n;i++) {
                if (i==root_idx) continue;
                if (in[i] == std::numeric_limits<double>::infinity()) {
                    // no arborescence from root
                    return {};
                }
            }

            int cnt = 0;
            idc.assign(n, -1);
            vis.assign(n, -1);
            for (int i=0;i<n;i++) res += in[i];

            for (int i=0;i<n;i++) {
                int v = i;
                while (vis[v] != i && idc[v] == -1 && v != root_idx) {
                    vis[v] = i;
                    v = pre[v];
                }
                if (v != root_idx && idc[v] == -1) {
                    for (int u = pre[v]; u != v; u = pre[u]) idc[u] = cnt;
                    idc[v] = cnt++;
                }
            }
            if (cnt == 0) {
                // Build final edge list with REAL weights from the original graph
                std::vector<Edge<T>> result;
                result.reserve(n-1);
                for (int v=0; v<n; ++v) {
                    if (v==root_idx) continue;
                    int u = pre[v];
                    if (u<0) continue;
                    const T from = nodes[u];
                    const T to   = nodes[v];
                    double w = get_weight_from_graph(from, to);
                    result.emplace_back(from, to, w);
                }
                return result;
            }

            for (int i=0;i<n;i++) if (idc[i] == -1) idc[i] = cnt++;

            std::vector<E> nes; nes.reserve(es.size());
            for (auto &e : es) {
                int u = idc[e.u], v = idc[e.v];
                double w = e.w;
                if (u != v) w -= in[e.v];
                nes.push_back({u, v, w});
            }
            es.swap(nes);
            n = cnt;
            root_idx = idc[root_idx];
        }
    }



   public:
    // ======================= SCC / CC =======================
    // Public facade: SCC for directed, CC for undirected
    std::vector<std::vector<T>> kosarajus_algorithm_scc(){
        return directed_ ? kosaraju_directed_impl()
                         : connected_components_impl();
    }

   private:
    using adj_list = std::unordered_map<T,std::unordered_set<std::pair<T,double>,pair_hash>>;

    adj_list transpose_graph_directed_() const {
        adj_list reversed;
        for(const auto& [v,_] : graph) reversed[v] = {};
        for(const auto& [u,values] : graph)
            for(auto& [v,w] : values)
                reversed[v].insert({u,w});
        return reversed;
    }

    void first_dfs(const T& vertex,std::stack<T>& stack_scc){
        std::stack<std::pair<T,bool>> st;
        std::unordered_set<T> vis;
        st.push({vertex,false});
        while(!st.empty()){
            auto [u,back] = st.top(); st.pop();
            if (back){ stack_scc.push(u); continue; }
            if (vis.count(u)) continue;
            vis.insert(u);
            st.push({u,true}); // postorder
            for(const auto& [nbr,_] : graph[u]) if(!vis.count(nbr)) st.push({nbr,false});
        }
    }

    void second_dfs(const T& vertex, const adj_list& gt,
                    std::unordered_set<T>& vis, std::vector<T>& comp){
        std::stack<T> st; st.push(vertex);
        vis.insert(vertex);
        while(!st.empty()){
            auto u = st.top(); st.pop();
            comp.push_back(u);
            for(const auto& [nbr,_] : gt.at(u))
                if(!vis.count(nbr)){ vis.insert(nbr); st.push(nbr); }
        }
    }

    std::vector<std::vector<T>> kosaraju_directed_impl(){
        std::stack<T> order;
        std::unordered_set<T> vis;
        for(const auto& [v,_] : graph) if(!vis.count(v)) first_dfs(v, order);

        auto gt = transpose_graph_directed_();
        vis.clear();
        std::vector<std::vector<T>> res;
        while(!order.empty()){
            T v = order.top(); order.pop();
            if (!vis.count(v)){
                std::vector<T> comp;
                second_dfs(v, gt, vis, comp);
                res.emplace_back(std::move(comp));
            }
        }
        return res;
    }

    std::vector<std::vector<T>> connected_components_impl() const {
        std::unordered_set<T> vis;
        std::vector<std::vector<T>> comps;
        for (const auto& [s,_] : graph) {
            if (vis.count(s)) continue;
            std::vector<T> comp;
            std::stack<T> st; st.push(s); vis.insert(s);
            while (!st.empty()) {
                T u = st.top(); st.pop();
                comp.push_back(u);
                for (const auto& [v,_w] : graph.at(u))
                    if (!vis.count(v)) { vis.insert(v); st.push(v); }
            }
            comps.push_back(std::move(comp));
        }
        return comps;
    }

   public:
    // ======================= Max-Flow (Edmonds–Karp) =======================
    double edmon_karp_algorithm(const T& source, const T& sink){
        using residual_graph = std::unordered_map<T,std::vector<Edge<T>>>;
        auto convert_to_residual = [&](){
            residual_graph rs;
            for (const auto& [u, neighbors] : graph)
                for (const auto& [v, cap] : neighbors)
                    rs[u].emplace_back(u, v, cap, 0.0);
            for (const auto& [u, neighbors] : graph) {
                for (const auto& [v, cap] : neighbors) {
                    bool rev = false;
                    if (rs.count(v))
                        for (const auto& e : rs.at(v)) if (e.vertex_r == u){ rev=true; break; }
                    if (!rev) rs[v].emplace_back(v, u, 0.0, 0.0);
                }
            }
            return rs;
        };

        auto bfs = [&](const residual_graph& rs, std::unordered_map<T,T>& parent)->bool{
            std::unordered_set<T> vis;
            std::queue<T> q; q.push(source); vis.insert(source);
            while(!q.empty()){
                T u=q.front(); q.pop();
                auto it = rs.find(u);
                if (it==rs.end()) continue;
                for (const auto& e : it->second){
                    if (e.residual_capacity()>0 && !vis.count(e.vertex_r)){
                        parent[e.vertex_r]=u;
                        if (e.vertex_r==sink) return true;
                        vis.insert(e.vertex_r);
                        q.push(e.vertex_r);
                    }
                }
            }
            return false;
        };

        auto rs = convert_to_residual();
        double flow=0.0;
        std::unordered_map<T,T> parent;

        while(bfs(rs,parent)){
            double add = std::numeric_limits<double>::infinity();
            for (T v=sink; v!=source; v=parent[v]){
                T u = parent[v];
                for (const auto& e : rs[u])
                    if (e.vertex_r==v){ add = std::min(add, e.residual_capacity()); break; }
            }
            for (T v=sink; v!=source; v=parent[v]){
                T u = parent[v];
                for (auto &e : rs[u]) if (e.vertex_r==v){ e.current_flow += add; break; }
                for (auto &e : rs[v]) if (e.vertex_r==u){ e.current_flow -= add; break; }
            }
            flow += add;
            parent.clear();
        }
        return flow;
    }

    // ======================= Hamilton =======================
    const std::vector<T> hamilton_cycle(const T& start){
        std::vector<T> path; path.reserve(vertices_amount+1);
        std::unordered_set<T> vis;
        path.push_back(start); vis.insert(start);
        if (dfs_hamilton_impl(start,start,path,vis)) return path;
        return {};
    }

   private:
    bool has_edge(const T& a,const T& b)const{
        auto it = graph.find(a);
        if(it == graph.end()) return false;
        return std::any_of(it->second.begin(), it->second.end(),
                           [&](const auto& pr){ return pr.first == b; });
    }

    bool dfs_hamilton_impl(const T& v,
                           const T& start,
                           std::vector<T>& path,
                           std::unordered_set<T>& vis){
        if(path.size() == vertices_amount){
            if(has_edge(v,start)){ path.push_back(start); return true; }
            return false;
        }
        for(const auto& [nbr,_w]: graph[v]){
            if(!vis.count(nbr)){
                vis.insert(nbr);
                path.push_back(nbr);
                if (dfs_hamilton_impl(nbr,start,path,vis)) return true;
                path.pop_back();
                vis.erase(nbr);
            }
        }
        return false;
    }

   public:
    // ======================= Formatting helpers =======================
    std::string to_string_with_weights(bool as_capacity=false) const {
        std::ostringstream os;
        os << "{\n";
        for(const auto& [vertex,neighbors] : this->graph){
            os << " " << vertex << " : [ ";
            for(const auto& [adjacent,weight] : neighbors){
                if (directed_ && as_capacity)
                    os << "(" << adjacent << ", cap=" << weight << ") ";
                else
                    os << "(" << adjacent << ", w=" << weight << ") ";
            }
            os << "]\n";
        }
        os << "}";
        return os.str();
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

}; // namespace Graph_implementation
