#include "network_interface.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

int main(int argc, char** argv) {
    if(argc < 2){
        cout << "Usage:\n";
        cout << argv[0] << " random <vertices> <probability>\n";
        cout << argv[0] << " manual <vertices> <edge1_u> <edge1_v> ...\n";
        return EXIT_FAILURE;
    }

    string mode = argv[1];
    string msg;

    if(mode == "random" && argc >= 4) {
        int vertices = std::atoi(argv[2]);
        double p = std::atof(argv[3]);
        std::ostringstream oss;
        oss << "RANDOM " << vertices << " " << p << "\n";
        msg = oss.str();
    } 
    else if(mode == "manual" && argc >= 4) {
        std::ostringstream oss;
        oss << "MANUAL ";
        for(int i = 2; i < argc; ++i) {
            oss << argv[i];
            if(i+1 < argc) oss << " ";
        }
        oss << "\n";
        msg = oss.str();
    } 
    else {
        cerr << "Invalid arguments.\n";
        return EXIT_FAILURE;
    }

    // === Create socket and connect ===
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) { perror("socket"); return EXIT_FAILURE; }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(std::atoi(PORT));
    if(inet_pton(AF_INET, IP, &server.sin_addr) <= 0){
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if(connect(sockfd, (sockaddr*)&server, sizeof(server)) < 0){
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    cout << "Connected to server!\n";

    // === Send the message ===
    if(send(sockfd, msg.c_str(), msg.size(), 0) != (ssize_t)msg.size()) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // === Receive response ===
    char buf[1024];
    ssize_t n;
    while((n = recv(sockfd, buf, sizeof(buf)-1, 0)) > 0){
        buf[n] = '\0';
        cout << buf;
    }
    if(n < 0) perror("recv");

    close(sockfd);
    return EXIT_SUCCESS;
}
