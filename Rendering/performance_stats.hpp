#pragma once
#include <chrono>
#include <cstddef>

// CPU elapsed times include submission to SFML, not GPU execution time.
struct PerformanceSample {
    double frameMs = 0, physicsMs = 0, gridMs = 0, trailsMs = 0, previewMs = 0;
};

class PerformanceStats {
    PerformanceSample total;
    std::size_t frames = 0;
public:
    PerformanceSample average;
    double fps = 0;
    void add(const PerformanceSample& sample) {
        total.frameMs += sample.frameMs;
        total.physicsMs += sample.physicsMs;
        total.gridMs += sample.gridMs;
        total.trailsMs += sample.trailsMs;
        total.previewMs += sample.previewMs;
        ++frames;
        if (total.frameMs < 500) return;
        const double count = static_cast<double>(frames);
        average = {total.frameMs / count, total.physicsMs / count,
            total.gridMs / count, total.trailsMs / count, total.previewMs / count};
        fps = 1000 * count / total.frameMs;
        total = {};
        frames = 0;
    }
};

template <typename Work>
double measureMilliseconds(Work work) {
    const auto start = std::chrono::steady_clock::now();
    work();
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
}
