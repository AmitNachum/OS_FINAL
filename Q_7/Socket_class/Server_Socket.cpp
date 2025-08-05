#include "Server_Socket.hpp"

void ServerSocketTCP::init(const char* ip, const char* port)
{
    struct addrinfo hints{},*res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if(int err = getaddrinfo(ip,port,&hints,&res)){
            std::cerr << "getaddrinfo: " << gai_strerror(err) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        this->listen_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        if(listen_fd < 0){
            freeaddrinfo(res);
            perror("socket");
            std::exit(EXIT_FAILURE);
        }

        int set_option = 1;
        setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&set_option,sizeof(set_option));


        if(bind(listen_fd,res->ai_addr,res->ai_addrlen) < 0){
            freeaddrinfo(res);
            perror("bind");
            close(listen_fd);
            std::exit(EXIT_FAILURE);
        }

        if(listen(listen_fd,BACKLOG) < 0){
            freeaddrinfo(res);
            perror("listen");
            close(listen_fd);
            std::exit(EXIT_FAILURE);
        }

      
        freeaddrinfo(res);

        make_non_blocking(listen_fd);
        file_descriptors.push_back({listen_fd,POLLIN,DEFAULT_REVENTS});
}

ServerSocketTCP::ServerSocketTCP(std::vector<pollfd> &fds,const char* ip , const char* port) 
: file_descriptors(fds)
{
    init(ip,port);
}

ServerSocketTCP::~ServerSocketTCP()
{   
    if(listen_fd >= 0)
        close(listen_fd);

}

int ServerSocketTCP::get_fd() const
{
    return listen_fd;
}

int ServerSocketTCP::make_non_blocking(int fd) const
{
    int flags = fcntl(fd,F_GETFL,0);
    return fcntl(fd,F_SETFD,flags | O_NONBLOCK);
}

std::vector<pollfd>&ServerSocketTCP::get_fds()
{
        return file_descriptors;
}

int ServerSocketTCP::accept_connections()
{
    sockaddr_storage client_addr;
    socklen_t len = sizeof(client_addr);
    int conn_fd = accept(listen_fd,(sockaddr*)&client_addr,&len);
    if(conn_fd >= 0){
        make_non_blocking(conn_fd);
        file_descriptors.push_back({conn_fd,POLLIN,0});
    }

    return conn_fd;
}



ssize_t ServerSocketTCP::send_to_client(int fd, const std::string &data) {
    size_t total = 0;
    const char *buf = data.data();
    size_t to_send = data.size();

    while (total < to_send) {
        ssize_t sent = ::send(fd, buf + total, to_send - total, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;  // interrupted, retry
            throw std::runtime_error(std::string("send failed: ")
                                     + std::strerror(errno));
        }
        if (sent == 0) break;  
        total += sent;
    }
    return total;
}

std::string ServerSocketTCP::recv_from_client(int fd, size_t max_bytes)
{
    std::string out;
    out.resize(max_bytes);

    ssize_t recvd = ::recv(fd, &out[0], max_bytes, 0);
    if (recvd < 0) {
        if (errno == EINTR) return recv_from_client(fd, max_bytes);
        throw std::runtime_error(std::string("recv failed: ")
                                 + std::strerror(errno));
    }
    if (recvd == 0) {
        // peer closed connection
        return {};
    }
    out.resize(recvd);
    return out;
}
