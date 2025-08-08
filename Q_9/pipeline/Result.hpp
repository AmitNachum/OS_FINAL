#pragma once
#include <string>

namespace Q9 {

// Enumeration for algorithm kinds used by the aggregator
enum class AlgoKind { MST, SCC, HAMILTON, MAXFLOW };

// Result passes a single algorithm's output to the aggregator
struct Result {
    std::string job_id;
    AlgoKind kind;
    bool ok = true;            // false for logical errors or unsupported cases
    std::string value;         // Text payload (format it as your client expects)
    std::string error_msg;     // Explanation if ok == false
};

} // namespace Q9