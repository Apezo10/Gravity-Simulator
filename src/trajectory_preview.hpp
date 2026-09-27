#pragma once

#include "planet_system.hpp"
#include "simulation_timing.hpp"
#include <vector>

struct PredictedPoint {
    double x, y;
};

class TrajectoryPreview {
public:
    static constexpr int days = 90;
    std::vector<std::vector<PredictedPoint>> paths;

    void calculate(const PlanetSystem& system) {
        // Forecast a copy so positions, velocities, and collisions stay untouched.
        PlanetSystem forecast = system;
        paths.assign(system.getBodies().size(), {});

        constexpr int steps = days * 86400 / PHYSICS_STEP_SECONDS;
        constexpr int sampleEvery = 12;

        for (int step = 1; step <= steps; ++step) {
            // End before a merge changes body indices or identities.
            if (!forecast.update(PHYSICS_STEP_SECONDS).empty()) {
                break;
            }

            if (step % sampleEvery != 0) {
                continue;
            }

            const auto& bodies = forecast.getBodies();

            for (std::size_t i = 0; i < bodies.size(); ++i) {
                paths[i].push_back({bodies[i].getX(), bodies[i].getY()});
            }
        }
    }
};
