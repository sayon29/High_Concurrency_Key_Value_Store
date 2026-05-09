#include "thread_pool.hpp"
#include "client_handler.hpp"

ThreadPool::ThreadPool(size_t num_threads, Store& store) 
    : stop(false), store_ref(store) {
    
    for(size_t i = 0; i < num_threads; ++i) {
        // Point the thread directly to the worker_loop function
        workers.push_back(std::thread(&ThreadPool::worker_loop, this));
    }
}

void ThreadPool::worker_loop() {
    while(true) {
        ClientConnection conn;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // Wait until the queue has a client, or we need to stop
            while(tasks.empty() && !stop) {
                condition.wait(lock);
            }

            if(stop && tasks.empty()) {
                return; // Exit thread cleanly
            }

            conn = tasks.front();
            tasks.pop();
        }

        // We are outside the lock now. Call the handler directly!
        ClientHandler::handle(conn.fd, store_ref, conn.ip);
    }
}

void ThreadPool::enqueue(int client_fd, const std::string& client_ip) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if(stop) return;
        
        ClientConnection conn;
        conn.fd = client_fd;
        conn.ip = client_ip;
        tasks.push(conn);
    }
    condition.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    
    for(std::thread &worker : workers) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}

ThreadPool::~ThreadPool() {
    if(!stop) shutdown();
}