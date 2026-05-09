
#include "client_handler.hpp"
#include "parser.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <cerrno>
#include "logger.hpp"

void ClientHandler::handle(int client_fd, Store &store, const std::string& client_ip) { 

    char buffer[1024];
    std::string pending;

    while(true) {

        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

        if(bytes < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            break;
        } 
        else if (bytes == 0) {
            break;
        }

        pending.append(buffer, bytes);

        while(true) {

            size_t pos = pending.find('\n');

            if(pos == std::string::npos) break;

            std::string request = pending.substr(0, pos);
            pending.erase(0, pos + 1);

            Command cmd = Parser::parse(request);

            std::string response;

            Logger::log(LogLevel::DEBUG, "[" + client_ip + "] Request: " + request);

            if(cmd.type == CommandType::SET) {
                store.set(cmd.key, cmd.value);
                response = "OK\n";
            }

            else if(cmd.type == CommandType::GET) {
                response = store.get(cmd.key) + "\n";
            }

            else if(cmd.type == CommandType::DEL) {
                store.del(cmd.key);
                response = "OK\n";
            }

            else if(cmd.type == CommandType::QUIT) {
                response = "GOODBYE\n";
                send(client_fd, response.c_str(), response.size(), 0);
                Logger::log(LogLevel::INFO, "Client [" + client_ip + "] requested disconnect.");
                
                close(client_fd); 
                return;
            }

            else {
                Logger::log(LogLevel::ERROR, "Invalid command from [" + client_ip + "]: " + request);
                response = "INVALID COMMAND\n";
            }

            send(client_fd, response.c_str(), response.size(), 0);
        }
    }

    Logger::log(LogLevel::INFO, "Client [" + client_ip + "] disconnected");
    close(client_fd);
}