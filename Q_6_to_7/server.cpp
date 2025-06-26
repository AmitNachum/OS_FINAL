#include "network_interface.hpp"


using std::cerr;
using std::cout;
using std::endl;
using namespace Graph_implementation;


static std::vector<pollfd> fds;

int main() {
    // Setup listener
    addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(IP, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }
    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        close(server_fd);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    freeaddrinfo(res);

    // Add listening socket to poll list
    fds.push_back({ .fd = server_fd, .events = POLLIN, .revents = 0 });

    cout << "[Server is listening on Port " << PORT << " TCP]" << endl;

    while (true) {
        int nready = ::poll(fds.data(), fds.size(), -1);
        if (nready < 0) {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < fds.size() && nready > 0; ++i) {
            auto &p = fds[i];
            if (p.revents & POLLIN) {
                --nready;

                if (p.fd == server_fd) {
                    // Accept new connection
                    sockaddr_storage client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd,
                                           (sockaddr*)&client_addr,
                                           &client_len);
                    if (client_fd < 0) {
                        perror("accept");
                        continue;
                    }
                    // Register and immediately read payload
                    fds.push_back({ .fd = client_fd, .events = POLLIN, .revents = 0 });
                    cout << "New connection established: fd=" << client_fd << endl;

                    char buf[BUF_SIZE];
                    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
                   if( n <= 0){
                    cout << "Client Disconnected." << endl;
                    close(client_fd);
                   } else{

                    buf[n] = '\0';
                    int vertices = 0;

                    if(sscanf(buf,"%d",&vertices) == 1){
                        cout <<"Received v= " << vertices << endl;
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<int> dist(0, vertices - 1);

                        auto graph = std::make_shared<Graph<int>>(vertices);

                        int vertex_arr[vertices];

                        for(int i = 0; i < vertices; ++i ) vertex_arr[i] = i + 1;

                            for(int i = 0; i < vertices; ++i){
                                for(int j = 0; j < vertices; ++j){
                                    if(i == j) continue;
                                    double weight = static_cast<double>(dist(gen));
                                    graph->add_edge(vertex_arr[i],vertex_arr[j],weight);
                                }
                            }

                            std::ostringstream oss;

                            oss << *graph << endl << endl;
                            try{
                            auto circut = graph->find_euler_circut();
                            oss << "Euiler circut" << endl;
                            oss << "[";
                            for(int v_c : *(circut)){
                                oss << v_c  << " ";
                            }
                            oss <<"]" << endl;
                            } catch(std::exception& ex){
                                cout << ex.what() << endl;
                            }


                            std::string graph_print = oss.str();


                            ssize_t sent = send(client_fd,graph_print.c_str(),graph_print.size(),0);

                            if(sent < 0) {
                                perror("send");
                            } else if ((size_t)sent < graph_print.size()) {
                                std::cerr << "Warning: only sent " << sent
                                << " of " << graph_print.size() << " bytes\n";
                            }
                       
                        
                    

                    
                    } 

                   }

                    

                } 
            }
        }

        // Remove closed sockets
        fds.erase(std::remove_if(fds.begin(), fds.end(),
                    [](const pollfd &p){ return p.events == 0; }),
                  fds.end());
    }

    for (auto &p : fds) close(p.fd);
    return EXIT_SUCCESS;
}

