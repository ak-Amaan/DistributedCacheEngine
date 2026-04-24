![C++](https://img.shields.io/badge/C++-17-blue)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey)
![Type](https://img.shields.io/badge/project-system--design-orange)

# 🚀 DistributedCacheEngine

> High-performance multi-threaded in-memory cache system in C++ with TTL, persistence, and query engine (Redis-inspired)

---

### ⚡ Highlights
- 🧠 O(1) average-time key-value operations
- 🔄 Multi-threaded TCP server (concurrent clients)
- ⏱️ TTL with lazy + background expiration
- 💾 Append-only persistence (AOF replay)
- 🔍 Custom query engine (`FIND WHERE value > x`)

`DistributedCacheEngine` is a modular C++17 cache server that combines an in-memory key-value store, a TCP command interface, TTL-based expiration, and append-only persistence. The project is structured like a production-oriented cache engine skeleton: the storage layer, parser, dispatcher, persistence, and networking concerns are separated so each piece can evolve independently.

The server listens on TCP port `8080`, accepts line-based cache commands, replays persisted mutations on startup, and can also expose a local interactive CLI when started from a terminal.

## 💡 Why This Project?

This project demonstrates how real-world cache systems like Redis are built internally, including:

- Memory-efficient data structures
- Concurrent request handling
- Persistence and recovery mechanisms
- Separation of system components for scalability

It focuses on systems-level engineering rather than UI-driven development.

## Project Overview

This project demonstrates the core building blocks behind a lightweight cache service:

- Fast in-memory reads and writes
- Command parsing and validation
- Request dispatching into a cache engine
- Background cleanup for expiring keys
- Append-only file (AOF) persistence for crash recovery
- Concurrent client handling over TCP

Although the project is named `DistributedCacheEngine`, the current implementation is a single-node cache server with a modular design that is ready for future replication, sharding, or clustering work.

## Features

- `SET`, `GET`, `DEL`, `PING`, and `HELP` command support
- `SETEX` for values with TTL in seconds
- `FIND WHERE value >|<|== <number>` for numeric value queries
- In-memory storage with thread-safe access
- Automatic expiration cleanup in a background thread
- Append-only persistence to `data/cache.aof`
- Replay of persisted mutations during startup
- TCP server with one handler thread per connected client
- Local interactive CLI when launched from a TTY
- Basic test suite covering storage, parsing, TTL behavior, queries, and persistence replay

## Architecture

The codebase is organized into focused modules:

- `main.cpp`
  Boots the application, restores persisted state, constructs the engine, starts the TCP server on port `8080`, and optionally starts an interactive CLI thread.

- `core/`
  Defines commands, the `CacheEngine`, and the `CommandDispatcher`.
  `CommandDispatcher` parses raw input and normalizes responses.
  `CacheEngine` owns command execution semantics and coordinates storage plus persistence.

- `parser/`
  Converts raw text commands into typed `Command` objects.
  The parser validates command shape before execution, including `SET`, `SETEX`, and `FIND WHERE ...` syntax.

- `storage/`
  Implements the in-memory key-value store and the storage abstraction.
  `Store` uses `std::shared_mutex` so read-heavy operations can proceed concurrently while writes and expiration cleanup still remain safe.

- `network/`
  Exposes a TCP server that accepts clients, reads newline-delimited requests, dispatches them, and writes responses back to the socket.

- `persistence/`
  Handles append-only logging and replay. Mutating commands are serialized to `data/cache.aof`, and the log is replayed on startup to rebuild state.

- `utils/`
  Includes input sanitization, tokenization, uppercase normalization, and colored structured logging.

### Request Flow

1. A client sends a single-line command over TCP or through the local CLI.
2. `CommandDispatcher` asks the parser to validate and translate that raw request.
3. `CacheEngine` executes the resulting command.
4. Mutating commands are first appended to the AOF log.
5. Storage is updated in memory.
6. A normalized response is returned to the client.

### Storage and TTL Behavior

- Normal keys are stored in memory until deleted.
- TTL keys created with `SETEX` store an expiration timestamp.
- Reads ignore expired entries.
- A background cleanup thread wakes periodically and removes expired keys proactively.

### Persistence Model

Only mutating commands are persisted:

- `SET`
- `SETEX`
- `DEL`

On startup, the AOF file is replayed in order. This keeps the persistence layer simple and easy to reason about while preserving the latest cache state across restarts.

## How To Run

### Prerequisites

- CMake `3.16+`
- A C++17-compatible compiler
- Optional: `readline` for enhanced interactive CLI support

### Build

From the project directory:

```bash
cmake -S . -B build
cmake --build build
```

### Run The Server

```bash
./build/cache_server
```

What happens on startup:

- The server replays `data/cache.aof` if it exists
- The TCP server starts on port `8080`
- If launched from a terminal, an interactive `cache>` prompt is also available locally

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Verified locally for this project: the build succeeds and `cache_engine_tests` passes.

## Example Commands

### Local Interactive CLI

After starting `./build/cache_server`, you can type:

```text
PING
SET region us-east
GET region
SETEX session 30 abc123
FIND WHERE value > 10
DEL region
HELP
```

Example responses:

```text
OK PONG
OK
VALUE us-east
OK
VALUE (empty)
OK deleted
OK Available commands:
```

### TCP Client Example

Using `nc` from another terminal:

```bash
printf "PING\n" | nc localhost 8080
printf "SET score 42\nGET score\n" | nc localhost 8080
printf "SETEX token 5 999\n" | nc localhost 8080
printf "FIND WHERE value > 20\n" | nc localhost 8080
printf "DEL score\n" | nc localhost 8080
```

Expected style of responses:

```text
OK PONG
OK
VALUE 42
OK
VALUE score
OK deleted
```

## Supported Commands

```text
HELP
PING
SET <key> <value>
GET <key>
DEL <key>
SETEX <key> <ttl_seconds> <value>
FIND WHERE value > <number>
FIND WHERE value < <number>
FIND WHERE value == <number>
```

## Design Decisions

### 1. Modular Boundaries First

The code separates parsing, dispatching, storage, networking, and persistence into distinct modules. That keeps the implementation maintainable and makes it easier to swap components later, such as replacing in-memory storage with a replicated backend.

### 2. In-Memory Store With Read-Optimized Locking

`Store` uses `std::shared_mutex` to support concurrent reads while preserving safe writes. This is a strong fit for cache workloads, which are usually read-heavy.

### 3. Simple Append-Only Persistence

The AOF design favors debuggability and recovery simplicity over compaction. Persisting raw mutating commands makes the log easy to inspect and replay.

### 4. TTL as a First-Class Cache Capability

Expiration is implemented both lazily on read and proactively in a cleanup thread. This hybrid model avoids stale reads while also preventing expired data from accumulating indefinitely.

### 5. Parser Validation Before Execution

Invalid requests are rejected before they reach the cache engine. This keeps execution logic simpler and centralizes command-shape validation in one layer.

### 6. Production-Oriented Skeleton

The codebase already includes pieces you would expect in a more complete cache server:

- explicit storage abstraction
- persistence boundary
- structured logging
- socket server lifecycle management
- tests for core behaviors

That makes the project a strong base for future work such as:

- replication
- sharding
- snapshotting
- authentication
- metrics and observability
- a true RESP protocol implementation

## Project Structure

```text
DistributedCacheEngine/
├── include/distributed_cache_engine/
├── src/
├── storage/
├── parser/
├── network/
├── persistence/
├── tests/
├── data/
├── main.cpp
└── CMakeLists.txt
```

## Notes

- The current networking protocol is line-based text commands.
- The `RespParser` type currently routes through the existing parser implementation rather than a full RESP frame parser.
- The current implementation is single-node despite the project name; distributed behavior would be a natural next phase rather than a feature already present.
