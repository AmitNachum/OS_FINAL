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
    // for printing/menu only (kept like before)
    std::unordered_map<int, bool> graph_direction;
    // NEW: per-client receive buffer for partial lines
    std::unordered_map<int, std::string> recv_buf;

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
                // read one chunk (non-blocking)
                std::string chunk = server.recv_from_client(fd);
                if (chunk.empty()) {
                    // client closed
                    ::close(fd);
                    client_graphs.erase(fd);
                    graph_direction.erase(fd);
                    recv_buf.erase(fd); // clear its pending bytes
                    fds.erase(fds.begin() + i);
                    --i;
                    std::cout << "Client " << fd << " disconnected.\n";
                    continue;
                }

                // append to this client's accumulator
                std::string& buf = recv_buf[fd];
                buf += chunk;

                // extract complete lines (newline-terminated)
                size_t start = 0;
                while (true) {
                    size_t nl = buf.find('\n', start);
                    if (nl == std::string::npos) break;

                    std::string rawline = buf.substr(start, nl - start);
                    start = nl + 1;

                    // normalize (strip CR if CRLF)
                    if (!rawline.empty() && rawline.back() == '\r') rawline.pop_back();

                    if (rawline.empty()) continue;
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

                        // Build the graph with the directed flag INSIDE the class
                        auto g = std::make_shared<Graph<Vertex>>(n, directed);
                        client_graphs[fd] = g;
                        graph_direction[fd] = directed; // for menu/printing only

                        std::cout << "Initialized graph for client " << fd
                                  << " with " << n << " vertices ("
                                  << (directed ? "directed" : "undirected") << ")\n";

                        // Always send a fresh menu right after init
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
                            // Add edge according to the graph's internal directed_ flag.
                            client_graphs[fd]->add_edge(u, v, w);
                        }
                        // No immediate response; the client will request an action next
                        continue;
                    }

                    // From here on we expect an action on an existing graph
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
                    // Parse the request using factory-compatible format.
                    // DO NOT block by graph type here — the Graph class dispatches internally.
                    Request<Vertex> req = parse_request<Vertex>(rawline, g);

                    try {
                        std::unique_ptr<AlgorithmIO<Vertex>> algo =
                            AlgorithmsFactory<Vertex>::create(req);

                        Response resp = algo
                            ? algo->run(req)
                            : Response{ false, std::string("Unknown algorithm: ") + cmd };

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
                    send_menu(server, fd);
                }

                // keep only the unconsumed tail (no newline yet)
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

    ::close(listen_fd);
    return 0;
}
