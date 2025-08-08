#pragma once
#include <string>

namespace Q9 {

// Outgoing represents a ready-to-send response to a client socket
struct Outgoing {
    int client_fd = -1;
    std::string payload;
};

} // namespace Q9