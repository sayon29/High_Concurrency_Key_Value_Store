#pragma once
#include <string>
#include <atomic>
#include <vector>
#include <thread>
#include <memory>
#include "store.hpp"
#include "event_loop.hpp" // Replaces thread_pool.hpp

class Server {
public:
    Server(int port, size_t num_threads);
    ~Server(); 
    void start(std::atomic<bool>& keep_running);

private:
    int port;
    int server_fd;
    
    Store store;     

    std::vector<std::unique_ptr<EventLoop>> loops;
    std::vector<std::thread> worker_threads;
};