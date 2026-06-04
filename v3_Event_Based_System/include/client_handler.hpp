#pragma once
#include <string>
#include "store.hpp"

class ClientHandler {
    public:
        static bool execute_command(int client_fd, const std::string& request, Store &store);
};