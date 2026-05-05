#pragma once

#include "kv_store.h"
#include <string>

namespace kv {

/**
 * CLI — Redis-like interactive command shell.
 *
 * Commands:
 *   SET <key> <value>   — store a key-value pair
 *   GET <key>           — retrieve value or NOT_FOUND
 *   DEL <key>           — remove a key
 *   EXISTS <key>        — true/false
 *   SIZE                — number of stored keys
 *   STATS               — hit rate, evictions, operation counts
 *   HELP                — print command list
 *   QUIT                — exit
 */
class CLI {
public:
    explicit CLI(KVStore& store);

    // Runs the interactive REPL until EOF or QUIT.
    void run();

private:
    // Processes one line. Returns false if the loop should exit.
    bool handleLine(const std::string& line);

    KVStore& store_;
};

} // namespace kv
