#pragma once
#include <string>
#include <functional>
#include <fstream>
namespace kv {
class WAL {
public:
    explicit WAL(const std::string& path);
    ~WAL();
    WAL(const WAL&) = delete;
    WAL& operator=(const WAL&) = delete;
    bool appendSet(const std::string& key, const std::string& value);
    bool appendDelete(const std::string& key);
    size_t replay(
        std::function<void(const std::string&, const std::string&)> on_set,
        std::function<void(const std::string&)> on_delete);
private:
    std::string path_;
    std::ofstream out_;
};
} // namespace kv
