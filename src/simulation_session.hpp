#pragma once
#include "planet_system.hpp"
#include "orbit_trail.hpp"
#include "simulation_timing.hpp"
#include <optional>

// Application state independent of the window and keyboard bindings.
class SimulationSession {
    std::vector<Planet> initialBodies;
    SimulationTiming timing;
    int stepsSinceTrailSample = 0;

    // Integer quarter-speed units represent 0.25x through 4x exactly.
    int speedQuarters = 4;

    void applyMerge(std::size_t survivorIndex, std::size_t removedIndex) {
        trails[survivorIndex] = OrbitTrail{
        };
        trails.erase(trails.begin() + removedIndex);


        if (!selected) {
            return;
        }


        // Follow the survivor, or shift an index affected by the removal.
        if (*selected == removedIndex) {
            selected = survivorIndex;
        } else if (*selected > removedIndex) {
            --*selected;
        }
    }

    void step(double seconds) {


        for (const auto& [survivorIndex, removedIndex] : system.update(seconds)) {
            applyMerge(survivorIndex, removedIndex);
        }

        elapsedSeconds += seconds;
        ++stepsSinceTrailSample;


        // Sample trails in physics time so rendering speed cannot change them.
        if (stepsSinceTrailSample == 4) {


            for (std::size_t i = 0; i < trails.size(); ++i) {
                trails[i].add(system.getBodies()[i]);
            }

            stepsSinceTrailSample = 0;
        }
    }

public:
    PlanetSystem system;
    std::vector<OrbitTrail> trails;
    bool paused = false;
    bool following = false;
    std::optional<std::size_t> selected;
    double elapsedSeconds = 0;

    explicit SimulationSession(const PlanetSystem& initial) : initialBodies(initial.getBodies()) {
        reset();
    }

    double speed() const {
        return speedQuarters / 4.0;
    }

    void faster() {
        speedQuarters = std::min(16, speedQuarters * 2);
    }

    void slower() {
        speedQuarters = std::max(1, speedQuarters / 2);
    }

    void reset() {
        system.setBodies(initialBodies);
        timing = SimulationTiming{
        };
        stepsSinceTrailSample = 0;
        speedQuarters = 4;
        paused = false;
        following = false;
        selected.reset();
        elapsedSeconds = 0;
        trails.assign(initialBodies.size(), OrbitTrail{
            });


        for (std::size_t i = 0; i < trails.size(); ++i) {
            trails[i].add(initialBodies[i]);
        }
    }

    void addAsteroid(double x, double y) {
        system.addAsteroid(x, y);
        trails.emplace_back();
        trails.back().add(system.getBodies().back());
    }

    void advance(std::int64_t microseconds) {


        // Discard paused wall time to prevent a catch-up jump on resume.
        if (paused) {
            return;
        }

        timing.advance(microseconds, [this](double seconds) {
                step(seconds);
            }, speedQuarters);
    }
};
