#pragma once
#include "planet_system.hpp"
#include "orbit_trail.hpp"
#include "simulation_timing.hpp"
#include <optional>

struct AsteroidLaunch {
    static constexpr double speedPerPixel = 300.0;
    double x, y;
    double vx = 0, vy = 0;
};

// Application state independent of the window and keyboard bindings.
class SimulationSession {
    std::vector<Planet> initialBodies;
    SimulationTiming timing;
    int stepsSinceTrailSample = 0;
    std::uint64_t revision = 0;

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
        ++revision;


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
    bool showPerformance = false;
    std::optional<std::size_t> selected;
    double elapsedSeconds = 0;
    std::optional<AsteroidLaunch> launch;

    explicit SimulationSession(const PlanetSystem& initial) : initialBodies(initial.getBodies()) {
        reset();
    }

    double speed() const {
        return speedQuarters / 4.0;
    }

    std::uint64_t stateRevision() const {
        return revision;
    }

    void faster() {
        speedQuarters = std::min(16, speedQuarters * 2);
    }

    void slower() {
        speedQuarters = std::max(1, speedQuarters / 2);
    }

    void reset() {
        ++revision;
        launch.reset();
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

    void addAsteroid(double x, double y, double vx = 0, double vy = 0) {
        ++revision;
        system.addAsteroid(x, y, vx, vy);
        trails.emplace_back();
        trails.back().add(system.getBodies().back());
    }

    void beginLaunch(double x, double y) {
        launch = AsteroidLaunch{x, y};
        ++revision;
    }

    void aimLaunch(double dx, double dy) {
        if (!launch) return;

        // Screen Y points down; world Y points up. Speed is independent of zoom.
        launch->vx = dx * AsteroidLaunch::speedPerPixel;
        launch->vy = -dy * AsteroidLaunch::speedPerPixel;
        ++revision;
    }

    void cancelLaunch() {
        launch.reset();
        ++revision;
    }

    void finishLaunch() {
        if (!launch) return;

        addAsteroid(launch->x, launch->y, launch->vx, launch->vy);
        cancelLaunch();
    }

    void advance(std::int64_t microseconds) {


        // Discard paused wall time to prevent a catch-up jump on resume.
        if (paused || launch) {
            return;
        }

        timing.advance(microseconds, [this](double seconds) {
                step(seconds);
            }, speedQuarters);
    }
};
