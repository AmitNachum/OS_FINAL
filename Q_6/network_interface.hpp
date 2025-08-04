#pragma once
#include <iostream>
#include <cstring>
#include <getopt.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <memory>
#include <cstdlib>
#include "../Q_1_to_4/Graph/Graph.hpp"
#include <vector>
#include <unordered_map>
#include <poll.h>
#include <arpa/inet.h> 
#include <algorithm>
#include <random>
#include <sstream>
#include <exception>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <algorithm>
#include <cctype>



#define PORT "5555"
#define IP "127.0.0.1"
#define BUF_SIZE 1024
#define BACKLOG 5
#define DEFAULT_REVENTS 0
#define NO_TIMEOUT -1

