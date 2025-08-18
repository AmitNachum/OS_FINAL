#include "../Q_7/Socket_class/Server_Socket.hpp"
#include "run_all.hpp"

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
#include <csignal>



namespace GI = Graph_implementation;
using Vertex = int;
using GraphT = GI::Graph<Vertex>;

volatile sig_atomic_t stop_flag = 0; // for graceful shutdown

static void handle_sigint(int) {
    std::cout << "\nServer received SIGINT. Shutting down gracefully";
    stop_flag = 1;
}   


// ------------------------- small string helpers -------------------------
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
// -----------------------------------------------------------------------

struct AlgoParams {
    std::optional<int> mf_source;
    std::optional<int> mf_sink;
    void reset() { mf_source.reset(); mf_sink.reset(); }
};

struct SharedState {
    // only the current leader touches fds (accept() pushes too)
    std::vector<pollfd> fds;

    // per-client state
    std::unordered_map<int, std::shared_ptr<GraphT>> client_graphs;/*Hash map to map client-> graph*/
    std::unordered_map<int, int>                     graph_n;/*client-> desired amount of vertices*/
    std::unordered_map<int, AlgoParams>              params;/*client -> Max_Flow Paramters*/

    // per-client input buffer (accumulate by '\n')
    std::unordered_map<int, std::string>             inbuf;

    // deferred closes (done by the leader)
    std::vector<int> pending_close;

    // leader-follower token
    std::mutex state_mtx;
    std::mutex leader_mtx;
    std::condition_variable leader_cv;
    bool leader_token   = false;
    bool shutting_down  = false;
};

// non-blocking recv helper: returns
//   - ""        -> peer closed
//   - string    -> payload
//   - "#NODATA" -> EAGAIN/EWOULDBLOCK (no data right now)
static inline std::string recv_nb(int fd) {
    char buf[4096];
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n > 0)  return std::string(buf, buf + n);
    if (n == 0) return std::string(); // closed
    if (errno == EAGAIN || errno == EWOULDBLOCK) return std::string("#NODATA");
    // treat other errors as closure for simplicity
    return std::string();
}



// ---------------------- one-line protocol handler ----------------------
static void process_line(const std::string& raw, int fd,
                         ServerSocketTCP& server, SharedState& S)
{
    if (raw.empty()) return;
    std::string line = raw; trim(line);
    if (line.empty()) return;

    std::string cmd = line.substr(0, line.find('|'));//Get the first command
    cmd = tolower_copy(cmd);

    // ---------------------- INIT ----------------------
    if (cmd == "init") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|');  // "init"
        std::getline(ss, tok, '|'); int n = std::stoi(tok);
        std::getline(ss, tok, '|'); bool directed = (tok == "1");

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.client_graphs[fd] = std::make_shared<GraphT>(n, directed);
            S.graph_n[fd] = n;
            S.params[fd].reset();
            S.inbuf[fd].clear();
        }
        std::cout << "Client " << fd << " init: n=" << n
                  << " (" << (directed ? "directed" : "undirected") << ")\n";
        return;
    }

    // ---------------------- EDGE ----------------------
    if (cmd == "edge") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|');  // "edge"
        std::getline(ss, tok, '|'); int u = std::stoi(tok);
        std::getline(ss, tok, '|'); int v = std::stoi(tok);
        std::getline(ss, tok, '|'); double w = std::stod(tok);

        std::shared_ptr<GraphT> g;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            auto it = S.client_graphs.find(fd);
            if (it != S.client_graphs.end()) g = it->second;
        }
        if (g) g->add_edge(u, v, w);
        else   server.send_to_client(fd, "ERR|Graph not initialized yet.\n");
        return;
    }

    // ------------- ONLY MAXFLOW PARAMS ARE ACCEPTED -------------
    if (cmd == "maxflow") {
        std::istringstream ss(line);
        std::string tok;
        std::getline(ss, tok, '|'); // "maxflow"
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

    // ignore other commands in stage 8
    if (cmd == "print" || cmd == "connected" || cmd == "scc" ||
        cmd == "mst"   || cmd == "hamilton") {
        return;
    }

    // ---------------------- COMMIT ----------------------
    if (cmd == "commit") {
        std::shared_ptr<GraphT> g;
        int n_for_flow = 0;
        std::optional<int> mf_src, mf_sink;

        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            auto it = S.client_graphs.find(fd);
            if (it != S.client_graphs.end()) g = it->second;
            auto itn = S.graph_n.find(fd);
            if (itn != S.graph_n.end()) n_for_flow = itn->second;

            auto pit = S.params.find(fd);
            if (pit != S.params.end()) {
                mf_src  = pit->second.mf_source;
                mf_sink = pit->second.mf_sink;
            }
        }

        if (!g) { server.send_to_client(fd, "ERR|Graph not initialized yet.\n"); return; }

        try {
            std::string ans = lf_stage8::run_all_algorithms(*g, n_for_flow, mf_src, mf_sink);
            server.send_to_client(fd, ans);
            server.send_to_client(fd,
                "You can send a new graph now.\n"
                "Start with: init|<n>|<directed:0/1>\n"
                "Then edges: edge|<u>|<v>|<w>\n"
                "Optionally set Max-Flow: maxflow|<source>|<sink>\n"
                "Finish with: commit\n");
        } catch (...) {
            try { server.send_to_client(fd, "Internal error while processing COMMIT.\n"); } catch (...) {}
        }
        return;
    }

    server.send_to_client(fd, std::string("ERR|Unknown command: ") + cmd + "\n");
}

// ---------------------- worker loop (Leader–Follower) ----------------------
static void worker_loop(ServerSocketTCP& server, SharedState& S) {
    const int listen_fd = server.get_fd();

    while (!stop_flag) {
        // acquire leader token
        {
            std::unique_lock<std::mutex> lk(S.leader_mtx);
            S.leader_cv.wait(lk, [&]{ return stop_flag || !S.leader_token || S.shutting_down; });
            if (stop_flag || S.shutting_down) return;
            S.leader_token = true;
        }

        // leader: perform deferred closes(removing any disconnected clients)
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            for (int fd : S.pending_close) {
                ::close(fd);
                auto it = std::find_if(S.fds.begin(), S.fds.end(),
                    [&](const pollfd& p){ return p.fd == fd; });//get a client iterator to remove it
                if (it != S.fds.end()) S.fds.erase(it);
                S.client_graphs.erase(fd);
                S.graph_n.erase(fd);
                S.params.erase(fd);
                S.inbuf.erase(fd);
                std::cout << "Client " << fd << " closed.\n";

            }
            S.pending_close.clear();
        }

        // poll
        int nready = poll(S.fds.data(), S.fds.size(), NO_TIMEOUT);
        if (nready < 0) {
            if (errno == EINTR) {//Interrupted Syscall(Mutating errno)
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
            }
            S.leader_cv.notify_all();
            return;
        }

        // pick one ready fd
        int chosen_fd = -1;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            for (auto& p : S.fds) {
                if (p.revents & POLLIN) { chosen_fd = p.fd; p.revents = 0; break; }
            }
        }
        //No Job is Available,then set a new leader
        if (chosen_fd == -1) {
            std::lock_guard<std::mutex> lk(S.leader_mtx);
            S.leader_token = false;
            S.leader_cv.notify_one();
            continue;
        }

        // accept path
        if (chosen_fd == listen_fd) {
            int cfd = server.accept_connections();
            if (cfd >= 0) {
                server.make_non_blocking(cfd);
                std::cout << "Client " << cfd << " connected.\n";
            }
            {
                std::lock_guard<std::mutex> lk(S.leader_mtx);
                S.leader_token = false;
            }
            S.leader_cv.notify_one();
            continue;
        }

        // release leadership; become worker for chosen_fd
        {
            std::lock_guard<std::mutex> lk(S.leader_mtx);
            S.leader_token = false;
        }
        S.leader_cv.notify_one();

        // read all available data; do NOT treat EAGAIN as close
        bool peer_closed = false;
        while (true) {
            std::string chunk = recv_nb(chosen_fd);
            if (chunk.empty()) { 
                peer_closed = true; 
                break;  // real close
            }        
            if (chunk == "#NODATA") break;                            // drained
            {
                std::lock_guard<std::mutex> lk(S.state_mtx);
                S.inbuf[chosen_fd] += chunk;
            }
            if (chunk.size() < 4096) break; // heuristic
        }

        if (peer_closed) {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            S.pending_close.push_back(chosen_fd);
            continue;
        }

        // process complete lines from buffer
        std::string local;
        {
            std::lock_guard<std::mutex> lk(S.state_mtx);
            local.swap(S.inbuf[chosen_fd]); //now local contains the inbuf[client_fd] data
        }

        size_t start = 0; // for ever 
        for (;;) {
            size_t pos = local.find('\n', start);
            //std::string::nops an indicator for an event "Not found".
            if (pos == std::string::npos) {//if '\n' was not found
                // put back remainder (partial line) to per-client buffer
                std::lock_guard<std::mutex> lk(S.state_mtx);
                S.inbuf[chosen_fd].append(local.substr(start));
                break;
            }
            std::string line = local.substr(start, pos - start); // Acumulating the Line successfully
            process_line(line, chosen_fd, server, S);
            start = pos + 1;
        }

    }
    
    return;
}

int main() {
    SharedState S;
    std::signal(SIGINT, handle_sigint);


    ServerSocketTCP server(S.fds);
    std::cout << "[Server listening on port " << PORT << " TCP]\n";

    const unsigned N = std::max(2u, std::thread::hardware_concurrency());//get the number of threads
    std::vector<std::thread> pool;
    pool.reserve(N);
    for (unsigned i = 0; i < N; ++i)
    //Filling a queue(in our case a vector) with the threads
    pool.emplace_back(worker_loop, std::ref(server), std::ref(S));

    {
        std::lock_guard<std::mutex> lk(S.leader_mtx);
        S.leader_token = false;
    }
    S.leader_cv.notify_one();

    for (auto& t : pool) if (t.joinable()) t.join();
    for (auto& p : S.fds) ::close(p.fd);
    return 0;
}
