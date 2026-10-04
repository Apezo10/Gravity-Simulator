#pragma once
#include "planet.hpp"
#include "collision_candidates.hpp"
#include <cstddef>
#include <utility>
#include <vector>

inline constexpr double G = 6.67430e-11;

class PlanetSystem {
    std::vector<Planet> planets;
    CollisionCandidates collisionCandidates;
    int asteroidCount = 0;
    bool accelerationsReady = false;
    double collisionKineticLoss = 0;

    void mergePair(std::size_t first, std::size_t second,
        std::vector<std::pair<std::size_t, std::size_t>>& merges);
    void mergeOverlaps(std::vector<std::pair<std::size_t, std::size_t>>& merges);
    void advancePositions(double dt,
        std::vector<std::pair<std::size_t, std::size_t>>& merges);
    void calculateAccelerations();
    double encounterStepLimit() const;
    void updateStep(double dt, std::vector<std::pair<std::size_t, std::size_t>>& merges);

public:
    void addAsteroid(double x, double y, double vx = 0, double vy = 0,
        double radius = 1000.0, double mass = 1.0e12);
    void setBodies(std::vector<Planet> bodies) {
        planets = std::move(bodies);
        asteroidCount = 0;
        collisionKineticLoss = 0;
        accelerationsReady = false;
    }

    std::vector<std::pair<std::size_t, std::size_t>> update(double dt);
    const std::vector<Planet>& getBodies() const {
        return planets;
    }
    double getCollisionKineticLoss() const { return collisionKineticLoss; }
};
