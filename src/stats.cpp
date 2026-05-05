#include "stats.h"
#include <sstream>
#include <iomanip>

namespace kv {

std::string Stats::report() const {
    std::ostringstream oss;
    oss << "--- Stats ---\n"
        << "  Gets:      " << gets()
        << "  (hits: " << hits() << ", misses: " << misses() << ")\n"
        << "  Hit rate:  " << std::fixed << std::setprecision(1)
        << hitRate() * 100.0 << "%\n"
        << "  Sets:      " << sets() << "\n"
        << "  Deletes:   " << deletes() << "\n"
        << "  Evictions: " << evictions() << "\n";
    return oss.str();
}

} // namespace kv
