#pragma once
#include "store.hpp"
#include <sys/epoll.h>
#include <unordered_map>
#include <string>

class EventLoop {
private:
    int epoll_fd;
    Store& store_ref;
    
    // The crucial state map: tracks the pending string buffer for each active socket
    std::unordered_map<int, std::string> client_buffers;

public:
    EventLoop(Store& store);
    ~EventLoop();

    // The Main Acceptor will call this to hand off a new connection
    void add_connection(int client_fd);

    // The infinite event loop run by the worker thread
    void run();
    
private:
    void handle_client_data(int client_fd);
    void disconnect_client(int client_fd);
};