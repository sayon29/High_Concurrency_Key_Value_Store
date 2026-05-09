#include "logger.hpp"

#ifdef ENABLE_LOGGING

#include <iostream>

std::ofstream Logger::file;

std::queue<std::string> Logger::q;

std::mutex Logger::mtx;

std::condition_variable Logger::cv;

std::thread Logger::worker;

bool Logger::running = false;

LogFilter Logger::filter = LogFilter::NORMAL;

bool Logger::should_log(LogLevel level) {

    if(level == LogLevel::ERROR) return true;

    if(filter == LogFilter::FULL) return true;

    // NORMAL mode
    return level != LogLevel::DEBUG;
}

void Logger::init(const std::string& filename, LogFilter f) {

    filter = f;
    file.open(filename, std::ios::app);

    running = true;
    worker = std::thread(thread_func);
}

void Logger::log(LogLevel level, const std::string& msg) {

    if(!should_log(level)) return;

    std::string prefix;

    if(level == LogLevel::DEBUG) prefix = "[DEBUG] ";
    else if(level == LogLevel::INFO) prefix = "[INFO ] ";
    else prefix = "[ERROR] ";

    std::string final_msg = prefix + msg;

    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(final_msg);
    }

    cv.notify_one();
}

void Logger::thread_func() {

    while(true) {

        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [] {
            return !q.empty() || !running;
        });

        if(!running && q.empty()) break;

        while(!q.empty()) {
            std::string msg = q.front();
            std::cout << msg << std::endl;
            file << q.front() << std::endl;
            q.pop();
        }
    }
}

void Logger::shutdown() {

    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }

    cv.notify_one();
    worker.join();

    file.close();
}

#else

// RELEASE MODE

void Logger::init(const std::string&, LogFilter) {}
void Logger::log(LogLevel, const std::string&) {}
void Logger::shutdown() {}

#endif