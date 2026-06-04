#include "client_handler.hpp"
#include "parser.hpp"
#include <sys/socket.h>
#include <string>

// Returns true to keep connection alive, false if the client wants to QUIT
bool ClientHandler::execute_command(int client_fd, const std::string& request, Store &store) { 
    Command cmd = Parser::parse(request);
    std::string response;

    if (cmd.type == CommandType::SET) {
        store.set(cmd.key, cmd.value);
        response = "OK\n";
    }
    else if (cmd.type == CommandType::GET) {
        response = store.get(cmd.key) + "\n";
    }
    else if (cmd.type == CommandType::DEL) {
        store.del(cmd.key);
        response = "OK\n";
    }
    else if (cmd.type == CommandType::QUIT) {
        response = "GOODBYE\n";
        send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
        return false; 
    }
    else {
        response = "INVALID COMMAND\n";
    }

    // Send the response back directly
    send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
    return true;
}