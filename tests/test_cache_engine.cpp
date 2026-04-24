#include "distributed_cache_engine/core/cache_engine.h"
#include "distributed_cache_engine/core/command.h"
#include "distributed_cache_engine/core/command_dispatcher.h"
#include "distributed_cache_engine/storage/in_memory_storage.h"
#include "persistence/aof_persistence.hpp"
#include "parser/parser.hpp"
#include "storage/store.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

using namespace std;

int main()
{
    dce::storage::Store store;
    store.set("alpha", "1");
    assert(store.get("alpha") == optional<string>{"1"});
    assert(store.del("alpha"));
    assert(!store.get("alpha").has_value());
    store.set("beta", "2", chrono::seconds(1));
    assert(store.get("beta") == optional<string>{"2"});
    this_thread::sleep_for(chrono::milliseconds(1200));
    assert(!store.get("beta").has_value());

    auto storage = make_shared<dce::storage::InMemoryStorage>();
    auto parser = make_shared<dce::parser::Parser>();

    dce::core::CacheEngine engine{storage};
    dce::core::CommandDispatcher dispatcher{parser, engine};

    assert(dispatcher.dispatch("HELP").find("OK Available commands:") == 0U);
    assert(dispatcher.dispatch("PING") == "OK PONG\n");
    assert(dispatcher.dispatch("SET region us-east") == "OK\n");
    assert(dispatcher.dispatch("GET region") == "VALUE us-east\n");
    assert(dispatcher.dispatch("GET region\r") == "VALUE us-east\n");
    assert(dispatcher.dispatch("GET region\033[A") == "VALUE us-east\n");
    assert(dispatcher.dispatch("DEL region") == "OK deleted\n");
    assert(dispatcher.dispatch("GET region") == "VALUE (nil)\n");
    assert(dispatcher.dispatch("SETEX session 1 token") == "OK\n");
    assert(dispatcher.dispatch("GET session") == "VALUE token\n");
    this_thread::sleep_for(chrono::milliseconds(1200));
    assert(dispatcher.dispatch("GET session") == "VALUE (nil)\n");
    assert(dispatcher.dispatch("SET score_low 10") == "OK\n");
    assert(dispatcher.dispatch("SET score_high 42") == "OK\n");
    assert(dispatcher.dispatch("SET score_mid 20") == "OK\n");
    assert(dispatcher.dispatch("SET label non_numeric") == "OK\n");
    assert(dispatcher.dispatch("FIND WHERE value > 15") == "VALUE score_high, score_mid\n");
    assert(dispatcher.dispatch("FIND WHERE value < 15") == "VALUE score_low\n");
    assert(dispatcher.dispatch("FIND WHERE value == 20") == "VALUE score_mid\n");
    assert(dispatcher.dispatch("FIND WHERE value == bad") == "ERROR FIND target must be an integer\n");
    assert(dispatcher.dispatch("NOOP") == "ERROR Invalid command. Type HELP for supported commands.\n");
    assert(!parser->parse("SET missing_value").has_value());
    assert(!parser->parse("SETEX broken ttl value").has_value());
    assert(!parser->parse("GET too many args").has_value());
    assert(parser->parse("HELP").has_value());
    assert(parser->parse("PING").has_value());
    assert(parser->parse("FIND WHERE value > 10").has_value());
    assert(!parser->parse("FIND value > 10").has_value());
    assert(!parser->parse("FIND WHERE value != 10").has_value());
    assert(!parser->parse("FIND WHERE key > 10").has_value());

    storage->set("session", "token", chrono::seconds(1));
    assert(storage->get("session") == optional<string>{"token"});
    this_thread::sleep_for(chrono::milliseconds(1200));
    assert(!storage->get("session").has_value());

    const filesystem::path aof_path =
        filesystem::temp_directory_path() / "distributed_cache_engine_test.aof";
    filesystem::remove(aof_path);

    auto persistent_storage = make_shared<dce::storage::InMemoryStorage>();
    auto persistence = make_shared<dce::persistence::AofPersistence>(aof_path.string());
    dce::core::CacheEngine persistent_engine{persistent_storage, persistence};
    dce::core::CommandDispatcher persistent_dispatcher{parser, persistent_engine};

    assert(persistent_dispatcher.dispatch("SET city 101") == "OK\n");
    assert(persistent_dispatcher.dispatch("SETEX token 5 999") == "OK\n");
    assert(persistent_dispatcher.dispatch("DEL city") == "OK deleted\n");

    auto recovered_storage = make_shared<dce::storage::InMemoryStorage>();
    assert(persistence->replay(*recovered_storage, *parser));
    assert(!recovered_storage->get("city").has_value());
    assert(recovered_storage->get("token") == optional<string>{"999"});

    filesystem::remove(aof_path);

    cout << "All DistributedCacheEngine tests passed.\n";
    return 0;
}
