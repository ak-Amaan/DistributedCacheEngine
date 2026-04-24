#pragma once

#include "distributed_cache_engine/core/command_dispatcher.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace dce::network
{
class Server
{
public:
    Server(std::uint16_t port, const core::CommandDispatcher& dispatcher);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    bool start();
    void stop();
    bool isRunning() const;

private:
    void acceptLoop();
    void handleClient(int client_socket);
    void closeAllClientSockets();

    std::uint16_t port_;
    const core::CommandDispatcher& dispatcher_;
    int server_socket_;
    std::atomic<bool> running_;
    std::thread accept_thread_;

    mutable std::mutex clients_mutex_;
    std::vector<int> client_sockets_;
    std::vector<std::thread> client_threads_;
};
} // namespace dce::network
