#include "trajectory_preview.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
int main() {
    std::vector<Planet> bodies{Planet(0, 0, 0, 0, 6.957e8, 1.9885e30)};
    for (int i = 0; i < 31; ++i) {
        double r = 1e11 + i * 1e10, a = i * 0.7;
        double v = std::sqrt(G * bodies[0].getMass() / r);
        bodies.emplace_back(r * std::cos(a), r * std::sin(a),
            -v * std::sin(a), v * std::cos(a), 1000, 1e12);
    }
    PlanetSystem system;
    system.setBodies(bodies);
    TrajectoryPreview preview;
    preview.begin(system);
    int frames = 0;
    double longest = 0;
    while (!preview.isComplete()) {
        auto start = std::chrono::steady_clock::now();
        preview.advance();
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        longest = std::max(longest, ms);
        ++frames;
    }
    std::cout << "32-body forecast: " << frames << " chunks, longest " << longest << " ms\n";
}
