#include "network_interface.hpp"
#include "../Q_1_to_4/Graph/Graph.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

using namespace Graph_implementation;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

static std::vector<pollfd> fds;

// === Helper: generate random unweighted graph ===
std::shared_ptr<Graph<int>> generate_random_graph(int vertices, double p) {
    auto graph = std::make_shared<Graph<int>>(vertices);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> prob(0.0, 1.0);

    for(int u=0; u<vertices; ++u){
        for(int v=u+1; v<vertices; ++v){
            if(prob(gen) <= p){
                graph->add_edge(u+1,v+1,0,false);
            }
        }
    }
    return graph;
}

// === Helper: send Euler circuit or a clear message if none ===
void handle_graph(std::shared_ptr<Graph<int>> graph, int client_fd) {
    std::ostringstream oss;
    oss << *graph << "\n";

    auto circut = graph->find_euler_circut();
    if(circut && !circut->empty()){
        oss << "Euler circuit found: [ ";
        for(int v : *circut) oss << v << " ";
        oss << "]\n";
        cout << "Sent Euler circuit to client fd=" << client_fd << endl;
    } else {
        oss << "No Euler circuit exists.\n";
        cout << "Graph for fd=" << client_fd << " has no Euler circuit\n";
    }

    string output = oss.str();
    send(client_fd, output.c_str(), output.size(), 0);
}

int main() {
    addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if(getaddrinfo(IP, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }
    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(server_fd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    // 🔹 Allow immediate reuse of port
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if(bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    if(listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    freeaddrinfo(res);

    fds.push_back({ .fd = server_fd, .events = POLLIN, .revents = 0 });
    cout << "[Server listening on Port " << PORT << " TCP]\n";

    while(true){
        int nready = poll(fds.data(), fds.size(), NO_TIMEOUT);
        if(nready < 0) { perror("poll"); break; }

        for(size_t i=0; i<fds.size() && nready>0; ++i){
            auto &p = fds[i];
            if(p.revents & POLLIN){
                --nready;
                if(p.fd == server_fd){
                    sockaddr_storage client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                    if(client_fd < 0){
                        perror("accept");
                        continue;
                    }
                    fds.push_back({ .fd = client_fd, .events = POLLIN, .revents = 0 });
                    cout << "New client connected, fd=" << client_fd << endl;
                } else {
                    char buf[BUF_SIZE];
                    ssize_t n = recv(p.fd, buf, sizeof(buf)-1, 0);
                    if(n <= 0){
                        close(p.fd);
                        p.fd = -1;
                        p.events = 0;
                        cout << "Client fd=" << p.fd << " disconnected\n";
                        continue;
                    }
                    buf[n] = '\0';
                    std::istringstream iss(buf);
                    string mode;
                    iss >> mode;

                    cout << "Client fd=" << p.fd << " sent: " << buf;

                    if(mode == "RANDOM") {
                        int vertices; double prob;
                        iss >> vertices >> prob;
                        auto graph = generate_random_graph(vertices, prob);
                        cout << "Generated random graph with " << vertices
                             << " vertices, p=" << prob << endl;
                        handle_graph(graph, p.fd);
                    } 
                else if(mode == "MANUAL") {
                    string line;
                    getline(iss, line); // get the rest of the line after "MANUAL"
                    std::istringstream ls(line);

                    // Parse vertices and expect a comma
                    int vertices;
                    char comma;
                    if(!(ls >> vertices >> comma) || comma != ',') {
                        string msg = "Invalid MANUAL format: expected <vertices>, ...\n";
                        send(p.fd, msg.c_str(), msg.size(), 0);
                        cout << msg;
                        continue;
                    }

                    if(vertices <= 0) {
                        string msg = "Invalid number of vertices.\n";
                        send(p.fd, msg.c_str(), msg.size(), 0);
                        cout << msg;
                        continue;
                    }

                    auto graph = std::make_shared<Graph<int>>(vertices);
                    std::unordered_set<string> edges_seen;
                    bool valid = true;

                    string edge_str;
                    while(ls >> edge_str) {
                        // Expect edge in the form u-v
                        size_t dash_pos = edge_str.find('-');
                        if(dash_pos == string::npos) {
                            valid = false;
                            cout << "Invalid edge format: " << edge_str << endl;
                            break;
                        }

                        int u = std::stoi(edge_str.substr(0,dash_pos));
                        int v = std::stoi(edge_str.substr(dash_pos+1));

                        // Validate vertex range and prevent loops
                        if(u < 1 || u > vertices || v < 1 || v > vertices || u == v) {
                            valid = false;
                            cout << "Invalid edge: " << edge_str 
                                << " for vertices count " << vertices << endl;
                            break;
                        }

                        // Check for duplicate edges (undirected graph)
                        string key1 = std::to_string(u) + "-" + std::to_string(v);
                        string key2 = std::to_string(v) + "-" + std::to_string(u);
                        if(edges_seen.count(key1) || edges_seen.count(key2)) {
                            valid = false;
                            cout << "Duplicate edge detected: " << edge_str << endl;
                            break;
                        }
                        edges_seen.insert(key1);

                        graph->add_edge(u,v,0,false);
                    }

                    if(!valid) {
                        string msg = "Invalid graph input\n";
                        send(p.fd, msg.c_str(), msg.size(), 0);
                        continue;
                    }

                    cout << "Received valid manual graph: " 
                        << vertices << " vertices, " 
                        << edges_seen.size() << " edges\n";

                    handle_graph(graph, p.fd);
                }
                }
            }
        }

        // Remove closed sockets
        fds.erase(std::remove_if(fds.begin(), fds.end(),
            [](const pollfd &p){ return p.fd==-1; }), fds.end());
    }

    for(auto &p : fds) if(p.fd!=-1) close(p.fd);
    return EXIT_SUCCESS;
}
