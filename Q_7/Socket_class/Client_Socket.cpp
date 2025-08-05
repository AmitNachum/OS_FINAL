#include "Client_Socket.hpp"

void ClientSocketTCP::init_client(const char *ip, const char *port)
{
        addrinfo hints{},*res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int err = getaddrinfo(ip,port,&hints,&res);
        if(err){
            std::cerr << "getaddrinfo: " << gai_strerror(err) << std::endl;
            std::exit(EXIT_FAILURE);
        }


        sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock_fd < 0) {
                perror("socket");
                freeaddrinfo(res);
                std::exit(EXIT_FAILURE);
        }

        if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
            perror("connect");
            freeaddrinfo(res);
            close(sock_fd);
            std::exit(EXIT_FAILURE);
        }
    freeaddrinfo(res);
}

void ClientSocketTCP::make_non_blocking() const {
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl - get flags");
        return;
    }

    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl - set non-blocking");
    }
}


void ClientSocketTCP::send_to_server(const std::string &msg) const{
    const char* buf = msg.data();
    size_t left = msg.size();
    while (left > 0) {
        ssize_t sent = send(sock_fd, buf, left, 0);
        if (sent < 0) {
            perror("send");
            return;
        }
        buf  += sent;
        left -= sent;
    }
}

std::string ClientSocketTCP::recv_from_server(size_t max_bytes) const
{
    std::string out(max_bytes,'\0');

    ssize_t n = recv(sock_fd,out.data(),max_bytes ,0);
    if( n < 0){
        perror("recv");
        return {};
    }

    out.resize(n);
    return out;

}

int ClientSocketTCP::get_fd() const
{
    return sock_fd;
}

ClientSocketTCP::ClientSocketTCP(const char* ip,const char* port){
    init_client(ip,port);
}

ClientSocketTCP::~ClientSocketTCP(){
    if(sock_fd >= 0){
        close(sock_fd);
    }
}