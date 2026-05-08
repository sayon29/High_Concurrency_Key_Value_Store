#pragma once

#include <string>
#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

enum class LogLevel {
    DEBUG,
    INFO,
    ERROR
};

enum class LogFilter {
    FULL,   // debug + info + error
    NORMAL  // info + error
};

class Logger {

private:

    static std::ofstream file;
    static std::queue<std::string> q;
    static std::mutex mtx;
    static std::condition_variable cv;
    static std::thread worker;
    static bool running;
    static LogFilter filter;

    static void thread_func();
    static bool should_log(LogLevel level);

public:

    static void init(const std::string& filename, LogFilter filter);

    static void log(LogLevel level, const std::string& msg);

    static void shutdown();
};