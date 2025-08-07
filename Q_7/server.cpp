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

using Vertex = int;

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
    // One graph per connected client
    std::unordered_map<int, std::shared_ptr<Graph<Vertex>>> client_graphs;
    // true = directed, false = undirected (affects validation & menu)
    std::unordered_map<int, bool> graph_direction;

    std::vector<pollfd> fds;
    ServerSocketTCP server(fds);
    int listen_fd = server.get_fd();

    std::cout << "[Server listening on port " << PORT << " TCP]" << std::endl;

    while (true) {
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
                // accept_connections() already pushed client_fd to fds inside ServerSocketTCP

                std::cout << "Client " << client_fd << " connected.\n";
                continue;
            }

            // Handle data from a client
            try {
                std::string msg = server.recv_from_client(fd);
                if (msg.empty()) {
                    // client closed
                    ::close(fd);
                    client_graphs.erase(fd);
                    graph_direction.erase(fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    std::cout << "Client " << fd << " disconnected.\n";
                    continue;
                }

                std::istringstream in(msg);
                std::string rawline;

                while (std::getline(in, rawline)) {
                    if (rawline.empty()) continue;

                    // Normalize a single line command
                    trim(rawline);
                    if (rawline.empty()) continue;

                    // Extract command token (before first '|') and normalize to lowercase
                    std::string cmd = rawline.substr(0, rawline.find('|'));
                    cmd = tolower_copy(cmd);

                    // ---------------------- INIT ----------------------
                    if (cmd == "init") {
                        std::istringstream ss(rawline);
                        std::string tok;
                        std::getline(ss, tok, '|');  // "init"
                        std::getline(ss, tok, '|'); int n = std::stoi(tok);
                        std::getline(ss, tok, '|'); bool directed = (tok == "1");

                        auto g = std::make_shared<Graph<Vertex>>(n);
                        client_graphs[fd] = g;
                        graph_direction[fd] = directed;

                        std::cout << "Initialized graph for client " << fd
                                  << " with " << n << " vertices ("
                                  << (directed ? "directed" : "undirected") << ")\n";

                        // Always send a fresh menu right after init
                        send_menu(server, fd, directed);
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
                            // Add according to the graph's directedness
                            client_graphs[fd]->add_edge(u, v, w, graph_direction[fd]);
                        }
                        // No immediate response; the client will request an action next
                        continue;
                    }

                    // From here on we expect an action on an existing graph
                    if (!client_graphs.count(fd)) {
                        server.send_to_client(fd, "ERR|Graph not initialized yet.\n");
                        send_menu(server, fd, false);
                        continue;
                    }

                    auto& g = *client_graphs[fd];
                    bool directed = graph_direction[fd];

                    // ---------------------- PRINT ----------------------
                    if (cmd == "print") {
                        // Use the existing operator<< to print the adjacency list with weights
                        std::ostringstream out;
                        out << g << "\n";
                        server.send_to_client(fd, out.str());
                        send_menu(server, fd, directed);
                        continue;
                    }

                    // ------------------ ALGORITHMS ---------------------
                    // Parse the request using factory-compatible format
                    Request<Vertex> req = parse_request<Vertex>(rawline, g);

                    // Validate per graph type BEFORE running
                    bool supported = true;
                    if ((cmd == "mst" || cmd == "euler") && directed) supported = false;
                    if (cmd == "maxflow" && !directed) supported = false;

                    if (!supported) {
                        server.send_to_client(fd, "ERR|This algorithm is not supported for the current graph type.\n");
                        send_menu(server, fd, directed);
                        continue;
                    }

                    try {
                        std::unique_ptr<AlgorithmIO<Vertex>> algo = AlgorithmsFactory<Vertex>::create(req);
                        Response resp = algo
                            ? algo->run(req)
                            : Response{ false, "Unknown algorithm: " + cmd };

                        // Guard against empty string responses from algorithms
                        if (resp.response.empty()) {
                            resp.ok = false;
                            resp.response = "Algorithm returned no output";
                        }

                        server.send_to_client(fd, serialize_response(resp));
                    } catch (const std::exception& e) {
                        server.send_to_client(fd, std::string("ERR|") + e.what() + "\n");
                    }

                    // Always end with the menu so the client knows the response is complete
                    send_menu(server, fd, directed);
                }

            } catch (...) {
                std::cerr << "Error handling client " << fd << ". Disconnecting.\n";
                ::close(fd);
                client_graphs.erase(fd);
                graph_direction.erase(fd);
                fds.erase(fds.begin() + i);
                --i;
            }
        }
    }

    ::close(listen_fd);
    return 0;
}
