#include "server.hpp"
#include "logger.hpp"
#include "event_loop.hpp"
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <sys/select.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

Server::Server(int port, size_t num_threads) 
    : port(port), store() { 
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd); // Main socket must be non-blocking

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Bind failed on port " + std::to_string(port));
    }

    // Initialize Event Loops
    for (size_t i = 0; i < num_threads; ++i) {
        loops.push_back(std::make_unique<EventLoop>(store));
        worker_threads.push_back(std::thread(&EventLoop::run, loops.back().get()));
    }
}

void Server::start(std::atomic<bool>& keep_running) {
    if(listen(server_fd, SOMAXCONN) == -1) throw std::runtime_error("Failed to listen");

    int main_epoll = epoll_create1(0);
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(main_epoll, EPOLL_CTL_ADD, server_fd, &ev);

    Logger::log(LogLevel::INFO, "Multi-Reactor Server started on port " + std::to_string(port));

    struct epoll_event events[10];
    size_t current_worker = 0;

    while(keep_running) {
        // Timeout of 1000ms so we can periodically check keep_running
        int num_events = epoll_wait(main_epoll, events, 10, 1000);

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                // Handle all queued connections
                while (true) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        continue;
                    }

                    set_nonblocking(client_fd);

                    // Round-robin assignment to worker threads
                    loops[current_worker]->add_connection(client_fd);
                    current_worker = (current_worker + 1) % loops.size();
                }
            }
        }
    }
    close(main_epoll);
    close(server_fd);
}

Server::~Server() {
    for (auto& t : worker_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}