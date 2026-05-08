# KV Store Server

A lightweight, multi-threaded key-value store server built in C++ using POSIX TCP sockets. This project explores core systems programming concepts, including socket communication, concurrency control, and asynchronous, thread-safe file logging.

---

## Features

- **Multi-threaded Architecture**  
  Uses a thread-per-client model to handle multiple concurrent TCP connections.

- **In-Memory Storage**  
  Thread-safe key-value operations protected by mutexes.

- **Supported Commands**  
  SET key value, GET key, and DEL key

- **Asynchronous Logging**  
  A dedicated background thread processes log queues to prevent disk I/O from blocking client requests.

- **Configurable Log Levels**
  - FULL → Debug, Info, Error  
  - NORMAL → Info, Error  
  - OFF → No logging (zero-overhead release builds)

- **Graceful Shutdown**  
  Handles SIGINT (Ctrl+C) and safely flushes logs and closes sockets.

---

## Project Architecture

- **Server**  
  Handles socket creation, binding, connection acceptance, and shutdown.

- **ClientHandler**  
  Manages per-client socket lifecycle and stream reading.

- **Parser**  
  Extracts and validates commands from raw TCP streams.

- **Store**  
  Thread-safe wrapper over std::unordered_map.

- **Logger**  
  Asynchronous logging system using queue + mutex + condition variable + file writer thread.

---

## Build Instructions

### Requirements
- C++17
- CMake 3.10+
- POSIX-compliant system (Linux/macOS)

---

### Build Steps

mkdir build  
cd build  
cmake ..  
make  

---

### Build Profiles

#### Standard Build (Default Logging)

cmake .. -DUSE_LOGGING=ON -DLOG_LEVEL=NORMAL  
make  

#### Debug Build (Verbose Logging)

cmake .. -DUSE_LOGGING=ON -DLOG_LEVEL=FULL  
make  

#### Release Build (No Logging Overhead)

cmake .. -DUSE_LOGGING=OFF  
make  

---

## Usage

### Start Server (default port 8080)

./server  

---

### Connect via netcat

nc 127.0.0.1 8080  

---

### Example Commands

SET user_1 sayon  
OK  

GET user_1  
sayon  

DEL user_1  
OK  

GET user_1  
NULL  

---

### Shutdown

Press Ctrl+C to stop the server.

The server will:
- Close sockets safely
- Flush pending logs
- Write logs to logs/server.log

---

## Future Roadmap

- Thread Pool instead of thread-per-client
- epoll-based event loop for scalability
- Persistent storage for crash recovery