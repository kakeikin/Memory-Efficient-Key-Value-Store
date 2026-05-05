#include "cli.h"
#include "common.h"
#include <iostream>
#include <sstream>

namespace kv {

CLI::CLI(KVStore& store) : store_(store) {}

void CLI::run() {
    std::cout << "kv-store v2.0  — type HELP for commands\n";
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (!handleLine(line)) break;
    }
    std::cout << "Goodbye.\n";
}

bool CLI::handleLine(const std::string& line) {
    if (line.empty()) return true;

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "SET") {
        std::string key, value;
        if (!(iss >> key) || !std::getline(iss >> std::ws, value) || value.empty()) {
            std::cout << "Usage: SET <key> <value>\n";
            return true;
        }
        if (store_.set(key, value)) {
            std::cout << "OK\n";
        } else {
            std::cerr << "ERROR: WAL write failed (disk full?)\n";
        }

    } else if (cmd == "GET") {
        std::string key;
        if (!(iss >> key)) { std::cout << "Usage: GET <key>\n"; return true; }
        auto val = store_.get(key);
        std::cout << (val ? *val : kv::NOT_FOUND_MSG) << "\n";

    } else if (cmd == "DEL" || cmd == "DELETE") {
        std::string key;
        if (!(iss >> key)) { std::cout << "Usage: DEL <key>\n"; return true; }
        std::cout << (store_.erase(key) ? "DELETED" : "NOT_FOUND") << "\n";

    } else if (cmd == "EXISTS") {
        std::string key;
        if (!(iss >> key)) { std::cout << "Usage: EXISTS <key>\n"; return true; }
        std::cout << (store_.exists(key) ? "true" : "false") << "\n";

    } else if (cmd == "SIZE") {
        std::cout << store_.size() << "\n";

    } else if (cmd == "STATS") {
        std::cout << store_.stats().report();

    } else if (cmd == "HELP") {
        std::cout <<
            "  SET <key> <value>   store a key-value pair\n"
            "  GET <key>           retrieve value (NOT_FOUND if missing)\n"
            "  DEL <key>           remove a key\n"
            "  EXISTS <key>        true/false\n"
            "  SIZE                number of stored keys\n"
            "  STATS               hit rate, evictions, counts\n"
            "  QUIT                exit\n";

    } else if (cmd == "QUIT" || cmd == "EXIT") {
        return false;

    } else {
        std::cout << "Unknown command '" << cmd << "'. Type HELP.\n";
    }
    return true;
}

} // namespace kv
