#pragma once


#include "../../Q_6/network_interface.hpp"


class ClientSocketTCP {
    private:
    int sock_fd = -1;
    void init_client(const char* ip, const char* port);
    void make_non_blocking() const;
    public:

    ClientSocketTCP(const char* ip = IP, const char* port = PORT);
    ~ClientSocketTCP();
    ClientSocketTCP(const ClientSocketTCP& other) = delete;
    ClientSocketTCP& operator=(ClientSocketTCP& other) = delete;
    void send_to_server(const std::string& msg)const;
    std::string recv_from_server(size_t max_bytes = BUF_SIZE)const;
    int get_fd() const;







};