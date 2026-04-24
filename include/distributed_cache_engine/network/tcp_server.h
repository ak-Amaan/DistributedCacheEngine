#pragma once

#include "distributed_cache_engine/core/command_dispatcher.h"
#include "distributed_cache_engine/network/server.h"

#include <atomic>
#include <cstdint>
#include <thread>

namespace dce::network
{
class TcpServer final : public Server
{
public:
    TcpServer(std::uint16_t port, const core::CommandDispatcher& dispatcher);
    ~TcpServer() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

private:
    void acceptLoop();
    void handleClient(int client_socket) const;

    std::uint16_t port_;
    const core::CommandDispatcher& dispatcher_;
    int server_socket_;
    std::atomic<bool> running_;
    std::thread accept_thread_;
};
} // namespace dce::network
