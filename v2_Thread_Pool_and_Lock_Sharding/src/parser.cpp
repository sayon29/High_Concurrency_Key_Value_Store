#include "parser.hpp"

#include <sstream>

Command Parser::parse(const std::string &input) {

    std::stringstream ss(input);

    std::string op;
    ss >> op;

    Command cmd;

    if(op == "SET") {
        cmd.type = CommandType::SET;
        ss >> cmd.key;
        ss >> cmd.value;
    }

    else if(op == "GET") {
        cmd.type = CommandType::GET;
        ss >> cmd.key;
    }

    else if(op == "DEL") {
        cmd.type = CommandType::DEL;
        ss >> cmd.key;
    }

    else if (op == "QUIT" || op == "EXIT") {
        return {CommandType::QUIT, "", ""};
    }

    else {
        cmd.type = CommandType::INVALID;
    }

    return cmd;
}