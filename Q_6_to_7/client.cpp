#include "network_interface.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char **argv) {

    bool gotv = false;
    int vertices = 0, opt;
    while ((opt = getopt(argc, argv, "v:e:")) != -1) {
        switch (opt) {
            case 'v': vertices = std::atoi(optarg); gotv = true; break;
            default:
                cerr << "Usage: " << argv[0]
                     << " -v <vertices> -e <edges>\n";
                return EXIT_FAILURE;
        }
    }

    if(!gotv){
        cout << " Requestig -v <vertices>" << endl;
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port   = htons(std::atoi(PORT));
    if (inet_pton(AF_INET, IP, &server.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&server), sizeof(server)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    cout << "Connected to server!" << endl;

    std::string msg = std::to_string(vertices) +
                                        " "    +
                                               +
                                        " "    ;
    if (send(sockfd, msg.c_str(), msg.size(), 0) != (ssize_t)msg.size()) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    cout << "Sent v=" << vertices << endl;

    char buf[1024];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        cout << buf;
    }
    if (n < 0) perror("recv");

    close(sockfd);
    return EXIT_SUCCESS;
}
