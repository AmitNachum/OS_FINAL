#include "../Q_6/network_interface.hpp"
#include "Factory/Factory_Algorithms.hpp"
#include "Strategy/Strategy_interface.hpp"
#include <unordered_map>
using Vertex = int;
using client_graph = std::unique_ptr<Graph<Vertex>>;
static std::vector<pollfd> file_descriptors;
using namespace Graph_implementation;
static std::unordered_map<int,client_graph> client_graphs;


int make_non_blocking(int fd);

int make_non_blocking(int fd){
    int flags = fcntl(fd,F_GETFL,0);
    return fcntl(fd,F_SETFL,flags |O_NONBLOCK);
}



int main(){

    int listen_fd;
    addrinfo hints{},*res;
    hints.ai_family = AF_INET;
    hints.ai_protocol = SOCK_STREAM;
    hints.ai_protocol = AI_PASSIVE;

   if (getaddrinfo(IP, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }
    listen_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(listen_fd < 0){
        freeaddrinfo(res);
        perror("socket");
        return EXIT_FAILURE;
    }

    if(bind(listen_fd,res->ai_addr,res->ai_addrlen) < 0){
        freeaddrinfo(res);
        perror("bind");
        return EXIT_FAILURE;
    }


    if(listen(listen_fd,BACKLOG) < 0){
        freeaddrinfo(res);
        close(listen_fd);
        perror("listen");
        return EXIT_FAILURE;
    }



    freeaddrinfo(res);

    file_descriptors.push_back({listen_fd,POLLIN,DEFAULT_REVENTS});


    std::cout << "[Server is listening on Port " << PORT <<" TCP]" <<std::endl;

    char buffer[BUF_SIZE];

    while (true){
        int nready = ::poll(file_descriptors.data(),file_descriptors.size(),NO_TIMEOUT);
        if (nready < 0) {
            perror("poll");
            break;
        }

        for(size_t i = 0; file_descriptors.size(); ++i){
            auto &p = file_descriptors[i];
            if(p.revents & POLLIN){
                --nready;

                if(p.fd == listen_fd){
                    int client_fd = accept(listen_fd,nullptr,nullptr);
                    if(client_fd >= 0){
                    make_non_blocking(client_fd);
                    file_descriptors.push_back({client_fd,POLLIN,DEFAULT_REVENTS});
                    std::cout <<"Client " <<client_fd << " Connected" << std::endl;
                   }
                } else {
                    int n = read(file_descriptors[i].fd,buffer,BUF_SIZE);
                    if(n <= 0){
                        close(file_descriptors[i].fd);
                        file_descriptors.erase(file_descriptors.begin() + 1);
                        --i;
                        continue;
                    }

                    std::string msg(buffer, n);
                    Request<Vertex> req = parese_request(msg);
                    
                    auto algo = AlgorithmsFactory<Vertex>::create(req);
                    Response resp;

                    if(!algo){
                        resp = {false,"Unknown Algorithm: " + req.name};
                    }
                    else{
                        resp = algo->run(req);
                    }


                std::string reply = (resp.ok ? "OK|" : "ERR|") + resp.ok;
                write(file_descriptors[i].fd, reply.c_str(), reply.size());
                }
            }
        }
    }
    
close(listen_fd);
return 0;
}