#include<iostream>
#include <csignal>
#include <atomic>
#include "server.hpp"
#include "logger.hpp"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        Logger::log(LogLevel::INFO, "Shutdown signal received. Stopping server");
        keep_running = false;
        Logger::shutdown();
    }
}

int main(){

    #ifdef LOG_FILTER_FULL
    Logger::init("logs/server.log", LogFilter::FULL);
    #else
    Logger::init("logs/server.log", LogFilter::NORMAL);
    #endif

    try {
        Server server(8080);
        server.start();
    }
    catch (const std::exception& e) {
        Logger::log(LogLevel::ERROR, e.what());
    }

    Logger::shutdown();
}