#include "distributed_cache_engine/network/tcp_server.h"

#include "distributed_cache_engine/utils/logger.h"
#include "distributed_cache_engine/utils/string_utils.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

namespace dce::network
{
namespace
{
constexpr int kBacklog = 16;
constexpr size_t kBufferSize = 4096;

bool sendAll(int socket_fd, const string& response)
{
    size_t total_sent = 0;
    while (total_sent < response.size())
    {
        const ssize_t bytes_sent = ::send(
            socket_fd,
            response.c_str() + total_sent,
            response.size() - total_sent,
            0
        );

        if (bytes_sent <= 0)
        {
            if (bytes_sent < 0 && errno == EINTR)
            {
                continue;
            }
            return false;
        }

        total_sent += static_cast<size_t>(bytes_sent);
    }

    return true;
}

void disableTcpBufferDelay(int socket_fd)
{
    int option = 1;
    if (::setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)) < 0)
    {
        utils::Logger::instance().error(
            "setsockopt(TCP_NODELAY) failed: " + string(strerror(errno))
        );
    }
}

bool tryPopRequestLine(string& pending, string& request)
{
    const size_t delimiter_pos = pending.find_first_of("\r\n");
    if (delimiter_pos == string::npos)
    {
        return false;
    }

    request = utils::sanitizeCommandInput(pending.substr(0, delimiter_pos));

    size_t erase_count = 1U;
    while (delimiter_pos + erase_count < pending.size() &&
           (pending[delimiter_pos + erase_count] == '\r' ||
            pending[delimiter_pos + erase_count] == '\n'))
    {
        ++erase_count;
    }
    pending.erase(0, delimiter_pos + erase_count);

    return true;
}
} // namespace

TcpServer::TcpServer(std::uint16_t port, const core::CommandDispatcher& dispatcher)
    : port_(port), dispatcher_(dispatcher), server_socket_(-1), running_(false)
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start()
{
    if (running_.load())
    {
        return true;
    }

    server_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0)
    {
        utils::Logger::instance().error("socket() failed");
        return false;
    }

    int opt = 1;
    if (::setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        utils::Logger::instance().error("setsockopt() failed");
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    if (::bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
        utils::Logger::instance().error(
            "bind() failed: " + string(strerror(errno))
        );
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    if (::listen(server_socket_, kBacklog) < 0)
    {
        utils::Logger::instance().error(
            "listen() failed: " + string(strerror(errno))
        );
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    running_.store(true);
    accept_thread_ = std::thread(&TcpServer::acceptLoop, this);
    return true;
}

void TcpServer::stop()
{
    const bool was_running = running_.exchange(false);
    if (!was_running)
    {
        return;
    }

    if (server_socket_ >= 0)
    {
        ::shutdown(server_socket_, SHUT_RDWR);
        ::close(server_socket_);
        server_socket_ = -1;
    }

    if (accept_thread_.joinable())
    {
        accept_thread_.join();
    }
}

bool TcpServer::isRunning() const
{
    return running_.load();
}

void TcpServer::acceptLoop()
{
    while (running_.load())
    {
        sockaddr_in client_address{};
        socklen_t client_length = sizeof(client_address);
        const int client_socket = ::accept(
            server_socket_,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length
        );

        if (client_socket < 0)
        {
            if (running_.load())
            {
                utils::Logger::instance().error(
                    "accept() failed: " + string(strerror(errno))
                );
            }
            continue;
        }

        disableTcpBufferDelay(client_socket);
        std::thread(&TcpServer::handleClient, this, client_socket).detach();
    }
}

void TcpServer::handleClient(int client_socket) const
{
    array<char, kBufferSize> buffer{};
    string pending;

    while (running_.load())
    {
        const ssize_t bytes_read = ::recv(client_socket, buffer.data(), buffer.size(), 0);
        if (bytes_read <= 0)
        {
            break;
        }

        pending.append(buffer.data(), static_cast<size_t>(bytes_read));

        string request;
        while (tryPopRequestLine(pending, request))
        {
            if (!request.empty())
            {
                const string response = dispatcher_.dispatch(request);
                if (!sendAll(client_socket, response))
                {
                    utils::Logger::instance().error(
                        "send() failed: " + string(strerror(errno))
                    );
                    ::close(client_socket);
                    return;
                }
            }
        }
    }

    ::close(client_socket);
}
} // namespace dce::network
