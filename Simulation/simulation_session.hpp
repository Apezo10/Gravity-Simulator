#pragma once
#include "planet_system.hpp"
#include "orbit_trail.hpp"
#include "simulation_timing.hpp"
#include <optional>
#include "accuracy_stats.hpp"

struct AsteroidLaunch {
    static constexpr double speedPerPixel = 300.0;
    double x, y;
    double vx = 0, vy = 0;
    double radius = 1000.0, mass = 1.0e12;

    void addTo(PlanetSystem& system) const {
        system.addAsteroid(x, y, vx, vy, radius, mass);
    }
};

// Application state independent of the window and keyboard bindings.
class SimulationSession {
    std::vector<Planet> initialBodies;
    SimulationTiming timing;
    int stepsSinceTrailSample = 0;
    std::uint64_t revision = 0;
    AccuracyStats accuracyBaseline;

    // Integer quarter-speed units represent 0.25x through 4x exactly.
    int speedQuarters = 4;
    double launchMass = 1.0e12;
    double launchRadius = 1000.0;

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


        const auto merges = system.update(seconds);
        for (const auto& [survivorIndex, removedIndex] : merges) {
            applyMerge(survivorIndex, removedIndex);
        }
        // A merge changes both kinetic and point-mass potential energy.
        // Start a new conservation interval rather than label that jump drift.
        if (!merges.empty()) accuracyBaseline = measureAccuracy(system.getBodies());

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
    bool showAccuracy = false;
    std::optional<std::size_t> selected;
    double elapsedSeconds = 0;
    std::optional<AsteroidLaunch> launch;

    explicit SimulationSession(const PlanetSystem& initial) : initialBodies(initial.getBodies()) {
        reset();
    }

    double speed() const {
        return speedQuarters / 4.0;
    }
    const AccuracyStats& getAccuracyBaseline() const { return accuracyBaseline; }

    std::uint64_t stateRevision() const {
        return revision;
    }

    void faster() {
        speedQuarters = std::min(16, speedQuarters * 2);
    }

    void slower() {
        speedQuarters = std::max(1, speedQuarters / 2);
    }

    double asteroidMass() const { return launchMass; }
    double asteroidRadius() const { return launchRadius; }

    // Bounded controls cover small projectiles through star-mass experiments.
    // Reject nonfinite inputs before clamping to keep invalid physics out.
    void setLaunchProperties(double mass, double radius) {
        if (!std::isfinite(mass) || !std::isfinite(radius) || mass <= 0 || radius <= 0) return;
        mass = std::clamp(mass, 1.0, 1.0e30);
        radius = std::clamp(radius, 1.0, 1.0e9);
        if (mass == launchMass && radius == launchRadius) return;
        launchMass = mass;
        launchRadius = radius;
        if (launch) {
            launch->mass = mass;
            launch->radius = radius;
        }
        ++revision;
    }

    void reset() {
        ++revision;
        launch.reset();
        system.setBodies(initialBodies);
        accuracyBaseline = measureAccuracy(system.getBodies());
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
        system.addAsteroid(x, y, vx, vy, launchRadius, launchMass);
        accuracyBaseline = measureAccuracy(system.getBodies());
        trails.emplace_back();
        trails.back().add(system.getBodies().back());
    }

    void beginLaunch(double x, double y) {
        launch = AsteroidLaunch{x, y, 0, 0, launchRadius, launchMass};
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

        launch->addTo(system);
        accuracyBaseline = measureAccuracy(system.getBodies());
        trails.emplace_back();
        trails.back().add(system.getBodies().back());
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
