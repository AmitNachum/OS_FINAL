#include "../Q_6/network_interface.hpp"
#include "./Socket_class/Client_Socket.hpp"
#include <iostream>
#include <string>
#include <random>
/*special keyword, if possible computation
values or function at compile time*/
static constexpr size_t CHUNK = 4096;
static const std::string MENU_SENTINEL = "Type 'exit' to disconnect.";

int main() {
    ClientSocketTCP client(IP, PORT);
    std::cout << "[Client] Connected to server.\n";

    int n, max_weight;
    double p;
    bool directed;
    std::string mode;

    std::cout << "Graph build mode (random/manual): ";
    std::cin >> mode;

    std::cout << "Number of vertices: ";
    std::cin >> n;

    std::cout << "Is graph directed? (1 = yes, 0 = no): ";
    std::cin >> directed;

    if (mode == "random") {
        std::cout << "Edge probability (0.0 - 1.0): ";
        std::cin >> p;
        std::cout << "Maximum edge weight (int): ";
        std::cin >> max_weight;

        /*Using a uniform Distribution for generating the Graph
        and its edges*/
        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        std::uniform_int_distribution<int> weight_dist(1, max_weight);

        client.send_to_server("init|" + std::to_string(n) + "|" + (directed ? std::string("1") : std::string("0")) + "\n");

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                if (prob_dist(rng) <= p) {
                    int w = weight_dist(rng);
                    client.send_to_server("edge|" + std::to_string(i) + "|" + std::to_string(j) + "|" + std::to_string(w) + "\n");
                    if (!directed)
                        client.send_to_server("edge|" + std::to_string(j) + "|" + std::to_string(i) + "|" + std::to_string(w) + "\n");
                }
            }
        }
    } else {
        client.send_to_server("init|" + std::to_string(n) + "|" + 
        (directed ? std::string("1") : std::string("0")) + "\n");

        std::cout << "Enter edges as: from to weight (enter -1 -1 -1 to stop):\n";
        while (true) {
            int u, v, w;
            std::cin >> u >> v >> w;
            if (u == -1 && v == -1) break;
            client.send_to_server("edge|" + std::to_string(u) + "|" + std::to_string(v) + "|" + std::to_string(w) + "\n");
            if (!directed)
                client.send_to_server("edge|" + std::to_string(v) + "|" + std::to_string(u) + "|" + std::to_string(w) + "\n");
        }
    }

    std::cout << "[Client] Graph sent. Receiving menu...\n";

    auto recv_until_menu = [&]() { // lambda function for message receiving 
        std::string buf, chunk;
        while (true) {
            chunk = client.recv_from_server(CHUNK);
            if (chunk.empty()) return std::string(); // disconnected
            buf += chunk;
            if (buf.find(MENU_SENTINEL) != std::string::npos) break;
        }
        return buf;
    };

    while (true) {
        std::string msg = recv_until_menu();
        if (msg.empty()) break;

        std::cout << msg << std::endl;
        std::cout << "[Client] > ";

        std::string input;
        if (std::cin.peek() == '\n') std::cin.ignore();//skip any '\n' escape sequence
        std::getline(std::cin, input);

        if (input == "exit") break;

        client.send_to_server(input + "\n");
    }

    std::cout << "Disconnected from server.\n";
    return 0;
}
