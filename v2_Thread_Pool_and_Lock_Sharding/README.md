# Concurrent KV Store System  
A high-performance concurrent key-value store built in C++ with a scalable multithreaded architecture. This version extends the base KV store with a worker thread pool, lock sharding, and integrated benchmarking tools to support massive concurrent workloads efficiently.

---

# Features

## 1. Thread Pool Connection Management
Instead of spawning one OS thread per client connection, the server maintains a fixed-size worker thread pool.

Incoming client sockets are pushed into a synchronized task queue, and worker threads continuously pull tasks from the queue and process them.

### Benefits
- Eliminates expensive thread creation/destruction overhead
- Reduces context switching under heavy load
- Improves scalability for thousands of concurrent clients
- Provides predictable resource usage

---

## 2. Lock-Sharded Storage (Partitioned Hash Map)
The in-memory key-value store is partitioned into multiple independent shards.

Each shard contains:
- Its own `std::unordered_map`
- Its own mutex

When a request is processed:
1. The key is hashed
2. The corresponding shard is selected
3. Only that shard is locked

This allows multiple threads to access different parts of the store simultaneously with minimal lock contention.

### Benefits
- Greatly improves parallel write throughput
- Reduces mutex bottlenecks
- Enables true concurrent access across worker threads

---

## 3. Integrated Benchmarking & Load Testing
The project includes custom benchmarking clients:
- `benchmark`
- `shard_benchmark`

These tools simulate large-scale concurrent traffic using:
- Multiple benchmark threads
- Thousands of simultaneous TCP connections
- Non-blocking sockets
- Poll-based event handling

### Metrics Collected
- Throughput (requests/sec)
- Average latency
- p50 latency
- p95 latency
- p99 latency
- Connection failures
- Timeout statistics

Benchmark reports are automatically written to:

```txt
results/benchmark_report.txt
```

---

# Build Instructions

The project uses CMake for building all executables.

## Requirements
- C++17 compatible compiler
- CMake 3.10+
- POSIX-compliant system (Linux/macOS)

---

# Standard Build (Logging Disabled)

Logging is disabled by default to maximize server performance.

```bash
mkdir build
cd build
cmake ..
make
```

---

# Build Profiles (Logging Configurations)

## Enable Standard Logging (INFO + ERROR)

```bash
cmake .. -DUSE_LOGGING=ON -DLOG_LEVEL=NORMAL
make
```

## Enable Verbose Logging (DEBUG + INFO + ERROR)

```bash
cmake .. -DUSE_LOGGING=ON -DLOG_LEVEL=FULL
make
```

---

# Running the Server

After compilation:

```bash
./server
```

---

# Running the Benchmark Tool

The `shard_benchmark` executable simulates high-concurrency traffic against the server.

```bash
./shard_benchmark [-c threads] [-r conns_per_thread] [-q reqs_per_conn] [-t poll_timeout_ms]
```

---

# Benchmark Configuration Flags

| Flag | Description | Default |
|------|-------------|----------|
| `-c` | Number of local benchmark threads | `4` |
| `-r` | Concurrent connections per thread | `250` |
| `-q` | Sequential requests sent per connection | `100` |
| `-t` | Poll timeout in milliseconds | `5000` |

---

# Example Benchmark Run

```bash
./shard_benchmark -c 8 -r 500 -q 200
```

This simulates:
- 8 benchmark threads
- 4,000 total concurrent connections
- 800,000 total requests

---

# Benchmark Report Interpretation

After completion, the benchmark prints a summary to the console and saves a detailed report to:

```txt
results/benchmark_report.txt
```

## Included Metrics

### Throughput
Total requests processed per second.

### Average Latency
Mean response time across all requests.

### p50 Latency
Median response time.

### p95 Latency
95% of requests complete within this time.

### p99 Latency
99% of requests complete within this time.

### Failure Metrics
Tracks:
- Connection drops
- Request failures
- Socket timeouts
- Server overload behavior

---

# Architecture Overview

```txt
Clients
   │
   ▼
Accept Thread
   │
   ▼
Synchronized Task Queue
   │
   ▼
Worker Thread Pool
   │
   ▼
Lock-Sharded KV Store
   ├── Shard 0 (mutex + hashmap)
   ├── Shard 1 (mutex + hashmap)
   ├── Shard 2 (mutex + hashmap)
   └── ...
```

---

# Project Goals
- High throughput under extreme concurrency
- Minimal lock contention
- Efficient thread utilization
- Low latency under heavy load
- Modular and extensible server architecture

---