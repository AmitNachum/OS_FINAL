#include "./Factory/Factory_Algorithms.hpp"
#include "./Socket_class/Server_Socket.hpp"
#include "./Strategy/Strategy_interface.hpp"   // Graph<int>
#include "server_functions.hpp"                 // parse_request + serialize_response
#include "processors.hpp"                       // send_menu(server, fd, directed)

#include <unordered_map>
#include <memory>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <string>
#include <csignal>   // <- for SIGINT

#ifndef GCOV_MODE
#define GCOV_MODE 0
#endif
using Vertex = int;

volatile sig_atomic_t stop_flag = 0;

// ---------------- Signal handling ----------------
static std::vector<int> all_fds;  // keep track of all sockets for cleanup

static void handle_sigint(int) {
    std::cout << "\nServer received SIGINT. Shutting down gracefully";
#if GCOV_MODE
    std::cout << " and flushing gcov data";
    __gcov_flush();
#endif
    stop_flag = 1;
}

// ---- helpers: trim / lowercase ------------------------------------------------
static inline void rtrim(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                          std::isspace(static_cast<unsigned char>(s.back()))))
        s.pop_back();
}
static inline void ltrim(std::string& s) {
    size_t i = 0;
    while (i < s.size() &&
           std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i) s.erase(0, i);
}
static inline void trim(std::string& s) { rtrim(s); ltrim(s); }

static inline std::string tolower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
} 
// -----------------------------------------------------------------------------


int main() {
    // Register signal handler for Ctrl+C
    std::signal(SIGINT, handle_sigint);

    // One graph per connected client
    std::unordered_map<int, std::shared_ptr<Graph<Vertex>>> client_graphs;
    // for printing/menu only
    std::unordered_map<int, bool> graph_direction;
    // per-client receive buffer for partial lines
    std::unordered_map<int, std::string> recv_buf;

    std::vector<pollfd> fds;
    ServerSocketTCP server(fds);
    int listen_fd = server.get_fd();

    // Track all sockets for cleanup on SIGINT
    all_fds.push_back(listen_fd);

    std::cout << "[Server listening on port " << PORT << " TCP]" << std::endl;

    while (!stop_flag) {
        int nready = poll(fds.data(), fds.size(), NO_TIMEOUT);
        if (nready < 0) {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < fds.size() && nready > 0; ++i) {
            auto& p = fds[i];
            if (!(p.revents & POLLIN)) continue;
            --nready;

            int fd = p.fd;

            // New connection
            if (fd == listen_fd) {
                int client_fd = server.accept_connections();
                if (client_fd < 0) continue;

                server.make_non_blocking(client_fd);
                all_fds.push_back(client_fd); // track for cleanup
                std::cout << "Client " << client_fd << " connected.\n";
                continue;
            }

            // Handle data from a client
            try {
                std::string chunk = server.recv_from_client(fd);
                if (chunk.empty()) {
                    // client closed
                    ::close(fd);
                    client_graphs.erase(fd);
                    graph_direction.erase(fd);
                    recv_buf.erase(fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    std::cout << "Client " << fd << " disconnected.\n";
#if GCOV_MODE
                    handle_sigint(0); // flush coverage and exit if enabled
#endif
                    continue;
                }

                std::string& buf = recv_buf[fd];
                buf += chunk;

                size_t start = 0;
                while (true) {
                    size_t nl = buf.find('\n', start);
                    if (nl == std::string::npos) break;

                    std::string rawline = buf.substr(start, nl - start);
                    start = nl + 1;

                    if (!rawline.empty() && rawline.back() == '\r') rawline.pop_back();
                    trim(rawline);
                    if (rawline.empty()) continue;

                    std::string cmd = rawline.substr(0, rawline.find('|'));
                    cmd = tolower_copy(cmd);

                    // ---------------------- INIT ----------------------
                    if (cmd == "init") {
                        std::istringstream ss(rawline);
                        std::string tok;
                        std::getline(ss, tok, '|');  // "init"
                        std::getline(ss, tok, '|'); int n = std::stoi(tok);
                        std::getline(ss, tok, '|'); bool directed = (tok == "1");

                        auto g = std::make_shared<Graph<Vertex>>(n, directed);
                        client_graphs[fd] = g;
                        graph_direction[fd] = directed;

                        std::cout << "Initialized graph for client " << fd
                                  << " with " << n << " vertices ("
                                  << (directed ? "directed" : "undirected") << ")\n";

                        send_menu(server, fd);
                        continue;
                    }

                    // ---------------------- EDGE ----------------------
                    if (cmd == "edge") {
                        std::istringstream ss(rawline);
                        std::string tok;
                        std::getline(ss, tok, '|');  // "edge"
                        std::getline(ss, tok, '|'); int u = std::stoi(tok);
                        std::getline(ss, tok, '|'); int v = std::stoi(tok);
                        std::getline(ss, tok, '|'); double w = std::stod(tok);

                        if (client_graphs.count(fd)) {
                            client_graphs[fd]->add_edge(u, v, w);
                        }
                        continue;
                    }

                    if (!client_graphs.count(fd)) {
                        server.send_to_client(fd, "ERR|Graph not initialized yet.\n");
                        send_menu(server, fd);
                        continue;
                    }

                    auto& g = *client_graphs[fd];

                    // ---------------------- PRINT ----------------------
                    if (cmd == "print") {
                        std::ostringstream out;
                        out << g << "\n";
                        server.send_to_client(fd, out.str());
                        send_menu(server, fd);
                        continue;
                    }

                    // ------------------ ALGORITHMS ---------------------
                    Request<Vertex> req = parse_request<Vertex>(rawline, g);

                    try {
                        std::unique_ptr<AlgorithmIO<Vertex>> algo =
                            AlgorithmsFactory<Vertex>::create(req);

                        Response resp = algo
                            ? algo->run(req)
                            : Response{ false, std::string("Unknown algorithm: ") + cmd };

                        if (resp.response.empty()) {
                            resp.ok = false;
                            resp.response = "Algorithm returned no output";
                        }

                        server.send_to_client(fd, serialize_response(resp));
                    } catch (const std::exception& e) {
                        server.send_to_client(fd, std::string("ERR|") + e.what() + "\n");
                    }

                    send_menu(server, fd);
                }

                if (start > 0) buf.erase(0, start);

            } catch (...) {
                std::cerr << "Error handling client " << fd << ". Disconnecting.\n";
                ::close(fd);
                client_graphs.erase(fd);
                graph_direction.erase(fd);
                recv_buf.erase(fd);
                fds.erase(fds.begin() + i);
                --i;
            }
        }

    }

    // Cleanup on normal exit
    client_graphs.clear();
    graph_direction.clear();
    recv_buf.clear();
    for (int fd : all_fds) if (fd != -1) ::close(fd);
    fds.clear();
    all_fds.clear();
    return 0;
}
