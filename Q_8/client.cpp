#include "../Q_6/network_interface.hpp"
#include "../Q_7/Socket_class/Client_Socket.hpp"

#include <iostream>
#include <string>
#include <random>

static constexpr size_t CHUNK = 4096;
static const std::string DONE_SENTINEL = "===== DONE =====";

int main() {
    try {
        ClientSocketTCP client(IP, PORT);
        std::cout << "[Client] Connected to server.\n";

        auto recv_until_done = [&]() -> std::string {
            std::string buf, chunk;
            while (true) {
                chunk = client.recv_from_server(CHUNK);
                if (chunk.empty()) break; // server closed
                buf += chunk;
                if (buf.find(DONE_SENTINEL) != std::string::npos) break;
            }
            return buf;
        };

        while (true) {
            // --- Collect graph parameters from user ---
            std::string mode;
            int n = 0, max_weight = 0;
            double p = 0.0;
            bool directed = false;

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

                // Send init
                client.send_to_server(
                    "init|" + std::to_string(n) + "|" + (directed ? std::string("1") : std::string("0")) + "\n"
                );

                // Generate and send edges
                std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
                std::uniform_int_distribution<int> weight_dist(1, max_weight);

                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < n; ++j) {
                        if (i == j) continue;
                        if (prob_dist(rng) <= p) {
                            int w = weight_dist(rng);
                            client.send_to_server(
                                "edge|" + std::to_string(i) + "|" + std::to_string(j) + "|" + std::to_string(w) + "\n"
                            );
                            if (!directed) {
                                client.send_to_server(
                                    "edge|" + std::to_string(j) + "|" + std::to_string(i) + "|" + std::to_string(w) + "\n"
                                );
                            }
                        }
                    }
                }
            } else {
                // Manual mode
                client.send_to_server(
                    "init|" + std::to_string(n) + "|" + (directed ? std::string("1") : std::string("0")) + "\n"
                );
                std::cout << "Enter edges as: from to weight (enter -1 -1 -1 to stop):\n";
                while (true) {
                    int u, v, w;
                    std::cin >> u >> v >> w;
                    if (u == -1 && v == -1) break;
                    client.send_to_server(
                        "edge|" + std::to_string(u) + "|" + std::to_string(v) + "|" + std::to_string(w) + "\n"
                    );
                    if (!directed) {
                        client.send_to_server(
                            "edge|" + std::to_string(v) + "|" + std::to_string(u) + "|" + std::to_string(w) + "\n"
                        );
                    }
                }
            }

            // Only Max-Flow parameters are needed
            int maxflow_src, maxflow_sink;
            std::cout << "Max-Flow source vertex: ";
            std::cin >> maxflow_src;
            std::cout << "Max-Flow sink vertex: ";
            std::cin >> maxflow_sink;
            client.send_to_server("maxflow|" + std::to_string(maxflow_src) + "|" + std::to_string(maxflow_sink) + "\n");

            // Finalize & get results
            client.send_to_server("commit\n");

            std::cout << "[Client] Waiting for aggregated results...\n";
            std::string reply = recv_until_done();
            if (reply.empty()) {
                std::cout << "[Client] Server closed the connection.\n";
                break;
            }
            std::cout << reply << std::endl;

            // Ask user to send another graph or exit
            std::cout << "Send another graph? (y/n): ";
            std::string again;
            std::cin >> again;
            if (again != "y" && again != "Y") {
                break;
            }
        }

        std::cout << "Disconnected from server.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[Client] Exception: " << e.what() << "\n";
        return 1;
    }
}
