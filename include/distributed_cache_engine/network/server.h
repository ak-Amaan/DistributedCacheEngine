#pragma once

namespace dce::network
{
class Server
{
public:
    virtual ~Server() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};
} // namespace dce::network
