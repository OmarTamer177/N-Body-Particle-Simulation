#ifndef N_BODY_TIMER_HPP
#define N_BODY_TIMER_HPP

#include <chrono>

// Lightweight wall-clock timer used by the sequential and OpenMP executables.
class Timer {
public:
    Timer() : start_(clock::now()) {}

    void reset() {
        start_ = clock::now();
    }

    double elapsed_seconds() const {
        const auto now = clock::now();
        return std::chrono::duration<double>(now - start_).count();
    }

private:
    using clock = std::chrono::steady_clock;
    clock::time_point start_;
};

#endif
