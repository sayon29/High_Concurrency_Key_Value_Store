#include "event_loop.hpp"
#include "client_handler.hpp"
#include "logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

#define MAX_EVENTS 64
#define READ_BUFFER_SIZE 1024

EventLoop::EventLoop(Store& store) : store_ref(store) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        Logger::log(LogLevel::ERROR, "Failed to create epoll instance in worker");
    }
}

EventLoop::~EventLoop() {
    close(epoll_fd);
}

void EventLoop::add_connection(int client_fd) {
    struct epoll_event ev{};
    // Level-triggered: epoll will keep notifying us as long as there is data to read
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR; 
    ev.data.fd = client_fd;
    
    // epoll_ctl is thread-safe on modern Linux
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    client_buffers[client_fd] = ""; // Initialize empty buffer
}

void EventLoop::run() {
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (num_events == -1) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            int fd = events[i].data.fd;

            // Handle hangups and errors
            if (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                disconnect_client(fd);
                continue;
            }

            // Handle incoming data
            if (events[i].events & EPOLLIN) {
                handle_client_data(fd);
            }
        }
    }
}

void EventLoop::handle_client_data(int client_fd) {
    char buffer[READ_BUFFER_SIZE];
    
    // Read all available bytes until EAGAIN
    while (true) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        
        if (bytes_read > 0) {
            client_buffers[client_fd].append(buffer, bytes_read);
        } else if (bytes_read == 0) {
            disconnect_client(client_fd);
            return;
        } else {
            // EWOULDBLOCK / EAGAIN means we've drained the OS read buffer
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; 
            } else {
                disconnect_client(client_fd);
                return;
            }
        }
    }

    // Now, process complete commands in the buffer
    auto& pending = client_buffers[client_fd];
    while (true) {
        size_t pos = pending.find('\n');
        if (pos == std::string::npos) break; // No complete command yet

        std::string request = pending.substr(0, pos);
        pending.erase(0, pos + 1);

        // Execute the command statelessly
        bool keep_alive = ClientHandler::execute_command(client_fd, request, store_ref);
        
        if (!keep_alive) {
            disconnect_client(client_fd);
            return;
        }
    }
}

void EventLoop::disconnect_client(int client_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    client_buffers.erase(client_fd);
}