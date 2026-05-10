#include "server.hpp"
#include "logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

// Initialize store first, then pass it to the pool
Server::Server(int port, size_t num_threads) 
    : port(port), store(), pool(num_threads, store) { 
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Bind failed on port " + std::to_string(port));
    }
}

void Server::start(std::atomic<bool>& keep_running) {
    if(listen(server_fd, SOMAXCONN) == -1) throw std::runtime_error("Failed to listen");

    struct timeval timeout;
    timeout.tv_sec = 1; 
    timeout.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    Logger::log(LogLevel::INFO, "Server started on port " + std::to_string(port));

    while(keep_running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if(client_fd == -1) continue; 

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        Logger::log(LogLevel::INFO, "Client connected from " + client_ip);

        pool.enqueue(client_fd, client_ip);
    }

    Logger::log(LogLevel::INFO, "Closing server socket...");
    close(server_fd);
    pool.shutdown(); 
}