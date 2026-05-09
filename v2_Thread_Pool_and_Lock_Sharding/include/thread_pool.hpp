#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include "store.hpp" // Needed for the Store reference

// Simple struct to hold the data, no lambdas required!
struct ClientConnection {
    int fd;
    std::string ip;
};

class ThreadPool {
public:
    // Takes the number of threads AND a reference to the server's Store
    ThreadPool(size_t num_threads, Store& store);
    ~ThreadPool();

    // Just pass the data in
    void enqueue(int client_fd, const std::string& client_ip);
    void shutdown();

private:
    // The actual function every thread runs
    void worker_loop(); 

    std::vector<std::thread> workers;
    std::queue<ClientConnection> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

    Store& store_ref; // Keep a reference so workers can pass it to ClientHandler
};