#pragma once
#include <cstddef>
#include <chrono>

namespace Q9 {

constexpr std::size_t Q_CAP_IN   = 128;
constexpr std::size_t Q_CAP_ALGO = 128;
constexpr std::size_t Q_CAP_AGG  = 256;
constexpr std::size_t Q_CAP_OUT  = 256;

constexpr int REQUIRED_RESULTS_PER_JOB = 4;

// Must match client.cpp DONE_SENTINEL from stage 8
static inline const char* RESPONSE_SENTINEL = "===== DONE =====";

using namespace std::chrono_literals;

} // namespace Q9
