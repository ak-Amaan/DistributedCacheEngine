#include "distributed_cache_engine/core/command.h"
#include "distributed_cache_engine/core/cache_engine.h"
#include "distributed_cache_engine/core/command_dispatcher.h"
#include "distributed_cache_engine/parser/resp_parser.h"
#include "distributed_cache_engine/storage/in_memory_storage.h"
#include "distributed_cache_engine/utils/logger.h"
#include "network/server.hpp"
#include "persistence/aof_persistence.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <unistd.h>

#ifdef USE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

using namespace std;

namespace
{
atomic<bool> g_shutdown_requested{false};

string color_green(const string& text)
{
    return "\033[32m" + text + "\033[0m";
}

string color_red(const string& text)
{
    return "\033[31m" + text + "\033[0m";
}

string color_cyan(const string& text)
{
    return "\033[36m" + text + "\033[0m";
}

string color_yellow(const string& text)
{
    return "\033[33m" + text + "\033[0m";
}

string cliPrompt()
{
    return "cache> ";
}

#ifdef USE_READLINE
string readlinePrompt()
{
    return "\001\033[36m\002" + cliPrompt() + "\001\033[0m\002";
}
#endif

void handleSignal(int)
{
    g_shutdown_requested.store(true);
}

bool startsWith(const string& text, const string& prefix)
{
    return text.rfind(prefix, 0) == 0U;
}

string colorResponse(const string& response)
{
    if (startsWith(response, "ERROR"))
    {
        return color_red(response);
    }

    if (startsWith(response, "OK") || startsWith(response, "VALUE"))
    {
        return color_green(response);
    }

    return color_yellow(response);
}

bool readCliLine(string& line)
{
#ifdef USE_READLINE
    char* input = readline(readlinePrompt().c_str());
    if (input == nullptr)
    {
        return false;
    }

    line = input;
    if (!line.empty())
    {
        add_history(input);
    }

    free(input);
    return true;
#else
    cout << color_cyan(cliPrompt()) << flush;
    return static_cast<bool>(getline(cin, line));
#endif
}

void runInteractiveCli(const dce::core::CommandDispatcher& dispatcher)
{
    cout << color_yellow("Interactive CLI ready. Type QUIT or EXIT to stop.\n");

    string line;
    while (!g_shutdown_requested.load())
    {
        if (!readCliLine(line))
        {
            break;
        }

        if (line == "QUIT" || line == "EXIT")
        {
            g_shutdown_requested.store(true);
            break;
        }

        if (line.empty())
        {
            continue;
        }

        cout << colorResponse(dispatcher.dispatch(line)) << flush;
    }
}
} // namespace

int main(int argc, char** argv)
{
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    (void)argc;
    (void)argv;

    auto storage = make_shared<dce::storage::InMemoryStorage>();
    auto parser = make_shared<dce::parser::RespParser>();
    auto persistence = make_shared<dce::persistence::AofPersistence>("data/cache.aof");

    if (!persistence->replay(*storage, *parser))
    {
        dce::utils::Logger::instance().error("Unable to replay AOF log");
        return 1;
    }

    dce::utils::Logger::instance().info(
        "Startup complete. Type HELP from any client for command usage."
    );

    dce::core::CacheEngine engine{storage, persistence};
    dce::core::CommandDispatcher dispatcher{parser, engine};
    dce::network::Server server{8080, dispatcher};

    if (!server.start())
    {
        dce::utils::Logger::instance().error("Unable to start server on port 8080");
        return 1;
    }

    dce::utils::Logger::instance().info(
        "DistributedCacheEngine TCP server listening on port 8080 with AOF persistence"
    );

    thread cli_thread;
    if (::isatty(STDIN_FILENO) != 0)
    {
        cli_thread = thread(runInteractiveCli, cref(dispatcher));
    }

    while (!g_shutdown_requested.load())
    {
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    dce::utils::Logger::instance().info("Shutdown requested");
    server.stop();
    if (cli_thread.joinable())
    {
        cli_thread.join();
    }
    return 0;
}
