#include "planet_system.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    std::vector<Planet> bodies{Planet(0, 0, 0, 0, 6.957e8, 1.9885e30)};
    for (int i = 0; i < 31; ++i) {
        const double radius = 1e11 + i * 1e10;
        const double angle = i * 0.7;
        const double speed = std::sqrt(G * bodies[0].getMass() / radius);
        bodies.emplace_back(radius * std::cos(angle), radius * std::sin(angle),
            -speed * std::sin(angle), speed * std::cos(angle), 1000, 1e12);
    }

    std::vector<double> times;
    double checksum = 0;
    for (int run = 0; run < 6; ++run) {
        PlanetSystem system;
        system.setBodies(bodies);
        const auto start = std::chrono::steady_clock::now();
        for (int step = 0; step < 4000; ++step) system.update(1800);
        const auto end = std::chrono::steady_clock::now();

        if (run > 0) times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        checksum = system.getBodies().back().getX();
    }

    std::sort(times.begin(), times.end());
    std::cout << "32 bodies, 4000 steps, median of 5 after warmup: "
              << times[2] << " ms; checksum: " << std::setprecision(17) << checksum << '\n';
}
