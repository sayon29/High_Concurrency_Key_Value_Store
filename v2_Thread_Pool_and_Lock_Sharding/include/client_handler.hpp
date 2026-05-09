#pragma once
#include <string>
#include "store.hpp"

class ClientHandler {
    public:
        static void handle(int client_fd, Store &store, const std::string& client_ip);
};