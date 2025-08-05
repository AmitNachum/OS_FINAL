
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

    void send_menu(ServerSocketTCP& server, int client_fd) {
        std::ostringstream menu;
        menu << "MENU|";
        for (size_t i = 0; i < ALGO_NAMES.size(); ++i) {
            menu << (i + 1) << ":" << ALGO_NAMES[i];
            if (i + 1 < ALGO_NAMES.size()) menu << ",";
        }
        menu << "\n";
        server.send_to_client(client_fd, menu.str());
    }