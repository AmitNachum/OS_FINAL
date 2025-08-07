
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <optional>

#include "../Q_1_to_4/Graph/Graph.hpp"
#include "./Factory/Factory_Algorithms.hpp"
#include "./Socket_class/Server_Socket.hpp"
#include "./Socket_class/Client_Socket.hpp"

const std::vector<std::string> ALGO_NAMES = {
    "euler",
    "hamilton",
    "mst",
    "scc",
    "maxflow"
};

inline void send_menu(ServerSocketTCP& server, int client_fd) {
    std::ostringstream menu;
    menu << "\n=== Algorithm Menu ===\n"
         << "Choose algorithm using format:\n"
         << "1) print       : print|||\n"
         << "2) euler       : euler|||\n"
         << "3) hamilton    : hamilton|<start_vertex>||\n"
         << "4) mst         : mst|<start_vertex>||\n"
         << "5) scc         : scc|||\n"
         << "6) maxflow     : maxflow||<source>|<sink>\n"
         << "Type 'exit' to disconnect.\n";
    server.send_to_client(client_fd, menu.str());
}
