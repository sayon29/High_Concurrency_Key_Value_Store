#include <iostream>
#include <csignal>
#include <atomic>
#include <unistd.h> 
#include <string>
#include "server.hpp"
#include "logger.hpp"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        keep_running = false;
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);

    int port = 8080;
    size_t num_threads = 100; // Default pool size

    int opt;
    while ((opt = getopt(argc, argv, "p:t:")) != -1) {
        switch (opt) {
            case 'p': port = std::stoi(optarg); break;
            case 't': num_threads = std::stoull(optarg); break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-p port] [-t threads]\n";
                return 1;
        }
    }

    #ifdef LOG_FILTER_FULL
    Logger::init("logs/server.log", LogFilter::FULL);
    #else
    Logger::init("logs/server.log", LogFilter::NORMAL);
    #endif

    try {
        Server server(port, num_threads);
        server.start(keep_running);
    }
    catch (const std::exception& e) {
        Logger::log(LogLevel::ERROR, e.what());
    }

    Logger::shutdown();
    return 0;
}