# Concurrent KV Store System (Version 3)

A highly scalable, concurrent key-value store built in C++ utilizing an epoll-based multi-reactor architecture.

Version 3 represents a complete overhaul of the server's network layer, abandoning the traditional synchronized task queue in favor of non-blocking I/O and independent event loops per worker thread. This design drastically improves CPU utilization and connection scalability under extreme loads.

---

# Deep Dive: Core Architecture

## 1. Multi-Reactor Event Loop

The system utilizes a multi-reactor pattern to distribute network load efficiently.

### Main Acceptor Thread

- Creates an epoll instance strictly to monitor the server listening socket.
- Accepts incoming client connections.
- Sets client file descriptors to non-blocking mode.
- Distributes connections to worker threads using round-robin assignment.

### Worker Threads

- The server spawns a configurable number of worker threads on startup (default: 100).
- Each worker runs its own `EventLoop` instance with a dedicated `epoll_fd`.

### Event Monitoring

Worker threads use `epoll_wait` to independently monitor up to 64 events simultaneously (`MAX_EVENTS`), listening for:

- `EPOLLIN`
- `EPOLLRDHUP`
- `EPOLLERR`

---

## 2. Fully Non-Blocking I/O & Stateful Buffering

To prevent slow clients from blocking worker threads, all socket operations are non-blocking.

### Aggressive Reading

When a socket becomes readable, the worker thread continuously reads into a 1024-byte stack buffer (`READ_BUFFER_SIZE`) until the kernel returns:

- `EAGAIN`
- `EWOULDBLOCK`

indicating that the socket receive buffer has been fully drained.

### Per-Client Buffering

Because TCP is a byte stream and may fragment messages:

- Incoming chunks are appended to a per-client buffer.
- Each client has an associated `std::string` buffer (`client_buffers[client_fd]`).

### Stateless Command Parsing

The event loop searches each client buffer for newline delimiters (`\n`).

- Complete commands are immediately dispatched to the `ClientHandler`.
- Partial commands remain safely buffered until additional data arrives.

### Efficient Writes

Responses are sent directly using:

```cpp
send(fd, data, len, MSG_NOSIGNAL);
```

This prevents `SIGPIPE` crashes when writing to disconnected clients.

---

## 3. Lock-Sharded In-Memory Storage

Global mutexes become severe bottlenecks under high contention.

Version 3 implements lock sharding:

- The key-value store is partitioned into multiple independent shards.
- Each shard contains:
  - Its own `std::unordered_map`
  - Its own `std::mutex`

### Request Processing

For every `SET`, `GET`, or `DEL`:

1. The key is hashed.
2. The shard index is computed via:

```cpp
get_shard_index(key);
```

3. Only the target shard is locked.

This allows multiple threads to operate on different keys in parallel with minimal contention.

---

## 4. Asynchronous Background Logging

Disk I/O is slow and should never block request processing.

The logging subsystem uses a dedicated background thread.

### Logging Pipeline

- Worker threads push log entries into a thread-safe queue.
- The queue is protected by a mutex.
- A condition variable wakes the logging thread whenever new messages arrive.

### Log Levels

Supported levels:

- `DEBUG`
- `INFO`
- `ERROR`

### Compile-Time Removal

Logging can be completely compiled out using:

```cpp
#ifdef ENABLE_LOGGING
```

allowing zero-overhead release builds.

---

# Build Instructions

CMake is required to build the project.

Because the server relies on `epoll`, a POSIX-compliant Linux environment is required.

```bash
mkdir build
cd build
```

---

## Release Build (Maximum Performance, Logging Disabled)

```bash
cmake .. -DUSE_LOGGING=OFF
make
```

---

## Standard Build (Info & Error Logs)

```bash
cmake .. -DUSE_LOGGING=ON -DLOG_LEVEL=NORMAL
make
```

---

# Usage & Configuration

Start the server using:

```bash
./server -p 8080 -t 8
```

### Command-Line Arguments

| Flag | Description | Default |
|--------|------------|---------|
| `-p` | Listening port | `8080` |
| `-t` | Number of EventLoop worker threads | `100` |

---

# Graceful Shutdown

Pressing `Ctrl+C` sends a `SIGINT` signal.

The server performs an orderly shutdown by:

1. Toggling the `keep_running` atomic flag.
2. Exiting all epoll event loops.
3. Joining worker threads.
4. Flushing pending log entries.
5. Writing remaining logs to:

```text
logs/server.log
```

before terminating.

---

# Advanced Load Testing

Version 3 includes a high-performance benchmarking utility:

```text
shard_benchmark
```

designed specifically to stress-test the epoll architecture.

---

## Benchmark Features

### Massive Concurrent Connections

Uses `poll()` multiplexing to manage thousands of non-blocking client connections across multiple benchmark threads.

### Zero Heap Allocations in the Hot Path

Request formatting uses a stack buffer:

```cpp
char req_buf[64];
```

to avoid dynamic memory allocation during benchmarking.

### Cache-Friendly Key Selection

The benchmark precomputes 700 random 4-byte keys and repeatedly accesses them, allowing the CPU to keep them resident in L1 cache and maximize throughput.

---

# Running the Benchmark

```bash
./shard_benchmark -c 8 -r 500 -q 200 -t 5000
```

### Benchmark Arguments

| Flag | Description | Default |
|--------|------------|---------|
| `-c` | Benchmark worker threads | `4` |
| `-r` | Concurrent connections per thread | `250` |
| `-q` | Sequential requests per connection before QUIT | `100` |
| `-t` | Poll timeout (milliseconds) | `5000` |

---

# Benchmark Reporting

After execution, the benchmark aggregates statistics from all worker threads and reports:

### Throughput

Total successfully processed requests per second.

### Latency Percentiles

- Average latency
- p50 (median)
- p95
- p99

### Failure Tracking

- Connection failures
- Dropped requests
- Timeouts (requests exceeding 30 seconds)

---

A detailed copy of the benchmark report is automatically appended to:

```text
../results/benchmark_report.txt
```