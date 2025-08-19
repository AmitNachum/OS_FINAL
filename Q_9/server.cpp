#include "../Q_7/Socket_class/Server_Socket.hpp"
// remove this for stage 9:
// #include "run_all.hpp"

// Q9 pipeline includes
#include "pipeline/Pipeline.hpp"
#include "pipeline/AlgoBinders.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cerrno>
#include <atomic>
#include <csignal> // for signal handling
#include <syncstream>
#define SCOUT std::osyncstream(std::cout)
std::mutex g_mtx; // global mutex for shared state



namespace GI = Graph_implementation;
using Vertex = int;
using GraphT = GI::Graph<Vertex>;
std::atomic<bool> stop_flag(false);// Global flag to stop the server
// ----------------------------------------------------------------

// ------------------------- helpers -------------------------
static inline void rtrim(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                          std::isspace(static_cast<unsigned char>(s.back()))))
        s.pop_back();
}
static inline void ltrim(std::string& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i) s.erase(0, i);
}
static inline void trim(std::string& s) { rtrim(s); ltrim(s); }
static inline std::string tolower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}
// -----------------------------------------------------------

struct AlgoParams {
    std::optional<int> mf_source;
    std::optional<int> mf_sink;
    void reset() { mf_source.reset(); mf_sink.reset(); }
};

struct SharedState {
    std::vector<pollfd> fds;

    std::unordered_map<int, std::shared_ptr<GraphT>> client_graphs;
    std::unordered_map<int, int>                      graph_n;
    std::unordered_map<int, AlgoParams>               params;
    std::unordered_map<int, std::string>              inbuf;

    std::vector<int> pending_close;

    std::mutex state_mtx;
    std::mutex leader_mtx;
    std::condition_variable leader_cv;
    bool leader_token  = false;
    bool shutting_down = false;
};


SharedState* gS = nullptr; // global pointer


// ---------------- Signal handling ----------------
static void handle_sigint(int) {
    std::cerr << "\nServer received SIGINT. Shutting down gracefully";
    stop_flag.store(true);
    if (gS) {
        std::lock_guard<std::mutex> lk(gS->leader_mtx);
        gS->leader_cv.notify_all();
    }
} 

static inline std::string recv_nb(int fd) {
    char buf[4096];
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n > 0)  return std::string(buf, buf + n);
    if (n == 0) return std::string(); // closed
    if (errno == EAGAIN || errno == EWOULDBLOCK) return std::string("#NODATA");
    return std::string();
}

// ---------------------- protocol handler ----------------------
static void process_line(const std::string& raw, int fd,
                         ServerSocketTCP& server, SharedState& S,
                         Q9::Pipeline& pipeline, std::atomic<uint64_t>& job_counter)
{
    if (raw.empty()) return;
    std::string line = raw; trim(line);
    if (line.empty()) return;

    std::string cmd = line.substr(0, line.find('|'));
    cmd = tolower_copy(cmd);

    if (cmd == "init") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|');
        std::getline(ss, tok, '|'); int n = std::stoi(tok);
        std::getline(ss, tok, '|'); bool directed = (tok == "1");

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.client_graphs[fd] = std::make_shared<GraphT>(n, directed);
            S.graph_n[fd] = n;
            S.params[fd].reset();
            S.inbuf[fd].clear();
        }
        SCOUT << "Client " << fd << " init: n=" << n
                  << " (" << (directed ? "directed" : "undirected") << ")\n";
        return;
    }

    if (cmd == "edge") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|');
        std::getline(ss, tok, '|'); int u = std::stoi(tok);
        std::getline(ss, tok, '|'); int v = std::stoi(tok);
        std::getline(ss, tok, '|'); double w = std::stod(tok);

        std::shared_ptr<GraphT> g;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            auto it = S.client_graphs.find(fd);
            if (it != S.client_graphs.end()) g =it->second;
        }
        if (g) g->add_edge(u, v, w);
        else   server.send_to_client(fd, "ERR|Graph not initialized yet.\n");
        
        
        return;
    }

    if (cmd == "maxflow") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|');
        int src=-1, sink=-1;
        if (std::getline(ss, tok, '|') && !tok.empty()) src  = std::stoi(tok);
        if (std::getline(ss, tok, '|') && !tok.empty()) sink = std::stoi(tok);
        if (src >= 0 && sink >= 0) {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.params[fd].mf_source = src;
            S.params[fd].mf_sink   = sink;
        }
        return;
    }

    if (cmd == "print" || cmd == "connected" || cmd == "scc" ||
        cmd == "mst"   || cmd == "hamilton") {
        return;
    }

    if (cmd == "commit") {
        std::shared_ptr<GraphT> g;
        int n_for_flow = 0;
        std::optional<int> mf_src, mf_sink;
        bool is_dir = true;

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            auto it = S.client_graphs.find(fd);
            if (it != S.client_graphs.end()) {
                g = it->second;
                is_dir = g->is_directed();
            }
            auto itn = S.graph_n.find(fd);
            if (itn != S.graph_n.end()) n_for_flow = itn->second;
            auto pit = S.params.find(fd);
            if (pit != S.params.end()) {
                mf_src  = pit->second.mf_source;
                mf_sink = pit->second.mf_sink;
            }
        }

        if (!g) { server.send_to_client(fd, "ERR|Graph not initialized yet.\n"); return; }

        // Defaults like stage 8 (0 .. n-1)
        const int default_s = 0;
        const int default_t = (n_for_flow > 0 ? n_for_flow - 1 : 0);
        
        Q9::Job job;//Creating a Job struct from the client's input
        job.client_fd = fd;
        job.job_id    = "J" + std::to_string(job_counter++);
        job.graph     = std::make_shared<GraphT>(*g);
        job.directed  = is_dir;
        job.s         = mf_src.has_value()  ? mf_src  : std::optional<int>(default_s);
        job.t         = mf_sink.has_value() ? mf_sink : std::optional<int>(default_t);

        pipeline.submit(job);

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.params[fd].reset();// Reset params for next job
        }
        return;
    }

    server.send_to_client(fd, std::string("ERR|Unknown command: ") + cmd + "\n");
}

// ---------------------- worker loop ----------------------
static void worker_loop(ServerSocketTCP& server, SharedState& S,
                        Q9::Pipeline& pipeline, std::atomic<uint64_t>& job_counter)
{
    const int listen_fd = server.get_fd();
    std::vector<pollfd> local_snap;

    while (!stop_flag.load()) {
        {
            //Acquire the leader token
            std::unique_lock<std::mutex> lk(S.leader_mtx);
            S.leader_cv.wait(lk, [&]{ return stop_flag.load() || !S.leader_token || S.shutting_down; });
            if (S.shutting_down || stop_flag.load()) return;
            S.leader_token = true;
        }

        

        {
            //Close any pending connections
            std::lock_guard<std::mutex> lk(S.state_mtx);
            for (int fd : S.pending_close) {
                ::close(fd);

                auto it = std::find_if(S.fds.begin(), S.fds.end(),
                    [&](const pollfd& p){ return p.fd == fd; });
                if (it != S.fds.end()) S.fds.erase(it);//Remove the client from the fds vector
                //Erase client-specific data
                S.client_graphs.erase(fd);
                S.graph_n.erase(fd);
                S.params.erase(fd);
                S.inbuf.erase(fd);
                SCOUT << "Client " << fd << " closed.\n";
            }
            S.pending_close.clear();
        }

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            local_snap = S.fds; // make a snapshot of current fds
        }

        constexpr int DEF_TIMEOUT = 100; // Poll timeout in ms

        int nready = poll(local_snap.data(), local_snap.size(), DEF_TIMEOUT);// Poll for events
        if (nready < 0) {
            if (errno == EINTR) {// Interrupted Syscall (Mutating errno)
                std::lock_guard<std::mutex> lk(S.leader_mtx);
                S.leader_token = false;
                S.leader_cv.notify_one();
                continue;
            }
            perror("poll");
            {
                std::lock_guard<std::mutex> lk(S.leader_mtx);
                S.shutting_down = true;
                S.leader_token = false;
                S.leader_cv.notify_all();
            }
            
            return;
        }

        int chosen_fd = -1;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            for (auto& p : local_snap) {
                if (p.revents & POLLIN) { chosen_fd = p.fd; p.revents = 0; break; }
            }
        }

        if (chosen_fd == -1) {
            std::lock_guard<std::mutex> lk(S.leader_mtx);
            S.leader_token = false;
            S.leader_cv.notify_one();
            continue;
        }

        if (chosen_fd == listen_fd) {//Incoming New connection
            int cfd = server.accept_connections();
            if (cfd >= 0) {
                server.make_non_blocking(cfd);
                SCOUT << "Client " << cfd << " connected.\n";
            }

            {
                std::lock_guard<std::mutex> lk(S.leader_mtx);
                S.leader_token = false;
                S.leader_cv.notify_one();
            }
           
            continue;
        }

        {
            std::lock_guard<std::mutex> lk(S.leader_mtx);
            S.leader_token = false;
            S.leader_cv.notify_one();
        }
        

        bool peer_closed = false;
        // read all available data; do NOT treat EAGAIN as close
        while (true) {
            std::string chunk = recv_nb(chosen_fd);
            if (chunk.empty()) { peer_closed = true; break; }
            if (chunk == "#NODATA") break;
            {
                std::lock_guard<std::mutex> lk(S.state_mtx);
                S.inbuf[chosen_fd] += chunk;// Append to the client's buffer
            }
            if (chunk.size() < 4096) break;
        }

        if (peer_closed) {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.pending_close.push_back(chosen_fd);
            continue;
        }

        // process complete lines from buffer
        // swap the inbuf to local to avoid holding the lock
        // and to allow concurrent processing of multiple clients
        std::string local;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            local.swap(S.inbuf[chosen_fd]);
        }

        // Process each line in the local buffer
        // This allows processing multiple commands from the same client in one go
        // Start from the beginning of the local buffer
        // and process until the end
        // If we reach the end, we will append any remaining data back to the inbuf
        size_t start = 0;
        for (;;) {
            size_t pos = local.find('\n', start);// Find the end of the line
            if (pos == std::string::npos) {// No more complete lines
                std::lock_guard<std::mutex> lk(S.state_mtx);
                S.inbuf[chosen_fd].append(local.substr(start)); // Append remaining data back to the inbuf
                break;//
            }
            // Extract the line
            // and process it
            std::string line = local.substr(start, pos - start);
            process_line(line, chosen_fd, server, S, pipeline, job_counter);
            start = pos + 1;
        }
    }
}

int main() {
    SharedState S;
    gS = &S; // Set the global pointer to the shared state

    // Register the signal handler for graceful shutdown
    signal(SIGINT, handle_sigint);
    
    ServerSocketTCP server(S.fds);
    SCOUT << "[Server listening on port " << PORT << " TCP]\n";

    // ---------- Stage 9: pipeline ----------
    std::atomic<uint64_t> job_counter{0};
    Q9::Pipeline pipeline(
        [&](int fd, const std::string& payload) {
            server.send_to_client(fd, payload);
        }
    );
    pipeline.set_mst_func(Q9::run_mst);
    pipeline.set_scc_func(Q9::run_scc);
    pipeline.set_ham_func(Q9::run_hamilton);
    pipeline.set_maxflow_func(Q9::run_maxflow);
    pipeline.start();// Start the pipeline  
    SCOUT << "Starting pipelining...." << std::endl;
    // ---------------------------------------

    const unsigned N = std::max(2u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    pool.reserve(N);
    for (unsigned i = 0; i < N; ++i)
        pool.emplace_back(worker_loop, std::ref(server), std::ref(S),
                             std::ref(pipeline), std::ref(job_counter));

    {
        std::lock_guard<std::mutex> lk(S.leader_mtx);
        S.leader_token = false;
        S.leader_cv.notify_one();
    }
    

    for (auto& t : pool) if (t.joinable()) t.join();
    for (auto& p : S.fds) ::close(p.fd);

    pipeline.stop();
    return 0;
}