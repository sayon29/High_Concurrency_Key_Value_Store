#pragma once

#include <string>

enum class CommandType {
    SET,
    GET,
    DEL,
    QUIT,
    INVALID
};

struct Command {
    CommandType type;
    std::string key;
    std::string value; // Only used for SET command
};

class Parser {
    public:
        static Command parse(const std::string &input);
};