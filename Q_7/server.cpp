#include "./Factory/Factory_Algorithms.hpp"
#include "./Socket_class/Server_Socket.hpp"
#include "./Strategy/Strategy_interface.hpp"   // for Graph<int>
#include "server_functions.hpp"                  // parse_request + serialize_response
#include <unordered_map>
#include <memory>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include "processors.hpp"


using Vertex = int;

int main() {
    std::unordered_map<int, std::shared_ptr<Graph<Vertex>>> client_graphs;
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
            auto &p = fds[i];
           
            if (!(p.revents & POLLIN)) continue;
            --nready;

            int fd = p.fd;
            if (fd == listen_fd) {
                int client_fd = server.accept_connections();
                if (client_fd < 0) continue;

                server.make_non_blocking(client_fd);
                fds.push_back({ client_fd, POLLIN, 0 });


                // Handshake: receive vertex count

                server.send_to_client(client_fd,"Enter the Amount of vertices");
                std::string init;
                try {
                    init = server.recv_from_client(client_fd);
                } catch (...) {
                    ::close(client_fd);
                    continue;
                }
                
                int nVertices = std::stoi(init);
                auto g = std::make_shared<Graph<Vertex>>(nVertices);
                client_graphs[client_fd] = g;

                // Build a directed clique (no self-edges)
                for (int v = 0; v < nVertices; ++v) {
                    for (int u = 0; u < nVertices; ++u) {
                        if (u == v) continue;
                        g->add_edge(v, u, 1.0, true);
                    }
                }

                
                std::cout << "Client " << client_fd
                          << " connected, graph of "
                          << nVertices << " vertices created" << std::endl;
                
                send_menu(server,client_fd);

                fds.push_back({ client_fd, POLLIN, 0 });
            } else {
   
                std::string msg = server.recv_from_client(fd);
                if (msg.empty()) {
                    ::close(fd);
                    client_graphs.erase(fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    std::cout << "Client " << fd << " disconnected" << std::endl;
                } else {
                    auto &g = *client_graphs[fd];
                    Request<Vertex> req = parse_request<Vertex>(msg, g);

                     std::unique_ptr<AlgorithmIO<Vertex>> algo = AlgorithmsFactory<Vertex>::create(req);
                    Response resp = algo
                        ? algo->run(req)
                        : Response{ false, "Unknown algorithm: " + req.name };

                    std::string wire = serialize_response(resp);
                    server.send_to_client(fd, wire);
                    send_menu(server,fd);
                }
            }
        }
    }

    ::close(listen_fd);
    return 0;
}
