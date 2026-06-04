# High-Performance C++ Key-Value Store

A highly scalable, concurrent key-value store server built in C++ using POSIX TCP sockets. This project represents a journey in systems programming, evolving from a basic thread-per-client model to a highly optimized, non-blocking, multi-reactor event loop architecture capable of handling extreme concurrent loads.

---

## Project Evolution

This repository tracks the evolution of the KV store across three major architectural versions:

* **Version 1 (The Foundation):** A simple, multi-threaded server using a thread-per-client model. It featured in-memory storage with global mutex locks, basic TCP stream parsing, and an asynchronous background logging system to prevent disk I/O blocking.
* **Version 2 (Thread Pool & Sharding):** Improved scalability by introducing a fixed-size worker thread pool and a synchronized task queue to eliminate thread creation overhead. It also introduced a lock-sharded storage system (partitioned hash maps) to drastically reduce lock contention during concurrent writes.
* **Version 3 (Multi-Reactor & epoll):** The current, state-of-the-art version. It ditches the synchronized task queue entirely in favor of an `epoll`-based event loop architecture. Connections are handled using non-blocking I/O across multiple reactor threads, maximizing CPU utilization and throughput.

---

## Global Features

* **In-Memory Storage:** Fast, robust KV storage supporting `SET`, `GET`, `DEL`, and `QUIT` commands.
* **Asynchronous Logging:** Dedicated background thread processing log queues to ensure zero-latency impact on client requests.
* **Custom Benchmarking:** Custom C++ load-testing clients simulating thousands of concurrent connections and tracking granular latency percentiles (p50, p95, p99).
* **Graceful Shutdown:** Safely catches `SIGINT`, flushes pending logs, and cleans up socket resources.

---

## Quick Start (Current Version)

### Requirements
* C++17 compatible compiler
* CMake 3.10+
* POSIX-compliant system (Linux preferred for `epoll`)

### Build Instructions
```bash
mkdir build
cd build
cmake .. -DUSE_LOGGING=OFF # Logging off for maximum performance
make
```

### Running the Server
```bash
./server -p 8080 -t 8
```