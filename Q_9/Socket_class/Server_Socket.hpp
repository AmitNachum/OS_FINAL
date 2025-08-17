#pragma once
#include "../../Q_7/network_interface.hpp"
#include <functional>



class ServerSocketTCP{
    private:
    int listen_fd = -1;
    std::vector<pollfd>& file_descriptors;
    void init(const char* ip, const char* port);

    public:
    ServerSocketTCP(std::vector<pollfd>& fds,const char* ip = IP,const char* port = PORT);
    ~ServerSocketTCP();
    ServerSocketTCP(const ServerSocketTCP& other) = delete;
    ServerSocketTCP& operator=(ServerSocketTCP &other) = delete;


    int get_fd()const;
    int make_non_blocking(int fd) const;
    std::vector<pollfd>& get_fds();
    int accept_connections();
    ssize_t send_to_client(int fd, const std::string& msg);
    std::string recv_from_client(int fd,size_t max_bytes = BUF_SIZE);




};