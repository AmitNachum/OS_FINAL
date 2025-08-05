#include "../Q_6/network_interface.hpp"
#include "./Socket_class/Client_Socket.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <chrono>
#include <thread>

int main() {
    // Connect to the server (IP and port are defined in network_interface.hpp)
    ClientSocketTCP client(IP, PORT);

    while (true) {
        // Receive a message from the server
        std::string msg = client.recv_from_server();
        if (msg.empty()) {
            std::cerr << "Server disconnected.\n";
            break;
        }

        // Display the message
        std::cout << msg << std::endl;

        // Read user input and send it back to the server
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            std::cout << "Disconnecting...\n";
            break;
        }
        std::cout << "[Client] sending: '" << input << "'\n";


        client.send_to_server(input);
        std::this_thread::sleep_for(std::chrono::seconds(1));
 
    }   

    return 0;
}
