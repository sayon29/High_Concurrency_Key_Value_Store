#pragma once
#include <string>
#include <atomic>
#include "store.hpp"
#include "thread_pool.hpp"

class Server {
public:
    Server(int port, size_t num_threads);
    void start(std::atomic<bool>& keep_running);

private:
    int port;
    int server_fd;
    Store store;     // MUST be declared before pool
    ThreadPool pool; // Needs Store reference
};