#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include "server.hpp"
#include "client_handler.hpp"
#include "logger.hpp"


void handle_client(int client_fd, Store& store, const std::string client_ip) {
    ClientHandler handler(client_fd, store, client_ip);
    handler.handle();
}

Server::Server(int port) : port(port) {
    
    //Initialize socket and bind to the specified port

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Server::start(){
    // Listen for incoming connections

    if(listen(server_fd, 5) == -1) {
        throw std::runtime_error("Failed to listen on socket");
    }

    Logger::log(LogLevel::INFO, "Server started on port " + std::to_string(port)); 

    while(true) {

        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if(client_fd == -1) continue;

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        std::string client_ip(ip_str);

        Logger::log(LogLevel::INFO, "Client connected from " + client_ip);

        std::thread t(handle_client, client_fd, std::ref(store), client_ip);

        t.detach();
    }

}