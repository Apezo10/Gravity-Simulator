#pragma once

#include "planet_system.hpp"
#include "simulation_timing.hpp"
#include <chrono>
#include <vector>

struct PredictedPoint {
    double x, y;
};

class TrajectoryPreview {
    PlanetSystem forecast;
    int completedSteps = 0;
    bool complete = true;
    // Original path indices are stable identities throughout this forecast.
    // A merged body carries all of its progenitors' paths forward.
    std::vector<std::vector<std::size_t>> pathOwners;

    static constexpr int sampleEvery = 12;

public:
    static constexpr int days = 90;
    static constexpr int steps = days * 86400 / PHYSICS_STEP_SECONDS;

    std::vector<std::vector<PredictedPoint>> paths;

    void begin(const PlanetSystem& system) {
        // Forecast a copy so positions, velocities, and collisions stay untouched.
        forecast = system;
        completedSteps = 0;
        complete = system.getBodies().empty();

        // Keep each path's storage between forecasts instead of reallocating it.
        paths.resize(system.getBodies().size());
        pathOwners.clear();
        pathOwners.reserve(paths.size());
        for (std::size_t i = 0; i < paths.size(); ++i) pathOwners.push_back({i});

        for (auto& path : paths) {
            path.clear();
            path.reserve(steps / sampleEvery);
        }
    }

    bool isComplete() const {
        return complete;
    }

    bool hasSamples() const {
        for (const auto& path : paths) if (!path.empty()) return true;
        return false;
    }

    void advance(std::chrono::microseconds budget = std::chrono::milliseconds(2),
        int maxSteps = steps) {
        const auto deadline = std::chrono::steady_clock::now() + budget;

        // Yield between physics steps so input and rendering can continue.
        // One individual physics step can still exceed the time budget.
        for (int i = 0; i < maxSteps && !complete; ++i) {
            if (std::chrono::steady_clock::now() >= deadline) {
                break;
            }

            // Replay merge events in order: each event uses the indices at
            // that point in the update, including multiple chained merges.
            for (const auto& [survivor, removed] : forecast.update(PHYSICS_STEP_SECONDS)) {
                auto& owners = pathOwners[survivor];
                owners.insert(owners.end(), pathOwners[removed].begin(), pathOwners[removed].end());
                pathOwners.erase(pathOwners.begin() + removed);
            }

            ++completedSteps;
            complete = completedSteps == steps;

            if (completedSteps % sampleEvery != 0) {
                continue;
            }

            const auto& bodies = forecast.getBodies();

            for (std::size_t i = 0; i < bodies.size(); ++i) {
                for (const auto owner : pathOwners[i]) {
                    paths[owner].push_back({bodies[i].getX(), bodies[i].getY()});
                }
            }
        }
    }
};
