#pragma once

#include <string>
#include "store.hpp"

class ClientHandler {
    private:
        int client_fd;
        Store &store;
        std::string client_ip;

    public:
        ClientHandler(int client_fd, Store &store, const std::string ip);
        void handle();
};