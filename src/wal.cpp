#include "wal.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

namespace kv {

WAL::WAL(const std::string& path) : path_(path) {
    // Create parent directories if they don't exist (e.g., "data/").
    std::filesystem::path p(path);
    if (p.has_parent_path() && !p.parent_path().empty()) {
        std::filesystem::create_directories(p.parent_path());
    }

    // Open in append mode so prior records are never overwritten.
    out_.open(path, std::ios::app);
    if (!out_.is_open()) {
        throw std::runtime_error("WAL: cannot open file: " + path);
    }
}

WAL::~WAL() {
    if (out_.is_open()) {
        out_.flush();
        out_.close();
    }
}

bool WAL::appendSet(const std::string& key, const std::string& value) {
    // Format: "SET <key> <value>\n"
    // Write before the in-memory update (write-ahead semantics).
    out_ << "SET " << key << " " << value << "\n";
    out_.flush(); // flush each record for crash durability
    return out_.good();
}

bool WAL::appendDelete(const std::string& key) {
    out_ << "DEL " << key << "\n";
    out_.flush();
    return out_.good();
}

size_t WAL::replay(
    std::function<void(const std::string&, const std::string&)> on_set,
    std::function<void(const std::string&)> on_delete)
{
    std::ifstream in(path_);
    if (!in.is_open()) {
        // A missing WAL means a fresh database — not an error.
        return 0;
    }

    size_t count = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string op;
        iss >> op;

        if (op == "SET") {
            std::string key, value;
            if (iss >> key && std::getline(iss >> std::ws, value)) {
                on_set(key, value);
                ++count;
            }
            // Malformed SET lines (missing key or value) are silently skipped.
        } else if (op == "DEL") {
            std::string key;
            if (iss >> key) {
                on_delete(key);
                ++count;
            }
        }
        // Unknown op codes are silently skipped for forward compatibility.
    }
    return count;
}

} // namespace kv
