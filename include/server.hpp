#pragma once
#include "store.hpp"

class Server {
    private:
        int port;
        int server_fd;
        Store store;
    public:
        Server(int port);    
        void start();
};