#pragma once
#include <algorithm>
#include <cstdint>

// 30 simulated hours per real second, in fixed 30-minute steps.
inline constexpr double PHYSICS_STEP_SECONDS = 1800.0;
class SimulationTiming {
    std::int64_t pendingTicks = 0;

public:
    template <typename Step>
    void advance(std::int64_t elapsedMicroseconds, Step step, int speedQuarters = 4) {

        // Limit catch-up after a stall to a quarter second (60 steps at 4x).
        // Discard excess wall time instead of freezing the UI in a backlog.
        pendingTicks += std::clamp<std::int64_t>(elapsedMicroseconds, 0, 250000) * 60
        * std::clamp(speedQuarters, 1, 16);


        while (pendingTicks >= 4000000) {
            step(PHYSICS_STEP_SECONDS);
            pendingTicks -= 4000000;
        }
    }
};
