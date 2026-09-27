#pragma once
#include "planet.hpp"
#include <cstddef>
#include <utility>
#include <vector>

inline constexpr double G = 6.67430e-11;

class PlanetSystem {
    std::vector<Planet> planets;
    int asteroidCount = 0;
    bool accelerationsReady = false;

    void mergeOverlaps(std::vector<std::pair<std::size_t, std::size_t>>& merges);
    void calculateAccelerations();

public:
    void addAsteroid(double x, double y, double vx = 0, double vy = 0);
    void setBodies(std::vector<Planet> bodies) {
        planets = std::move(bodies);
        asteroidCount = 0;
        accelerationsReady = false;
    }

    std::vector<std::pair<std::size_t, std::size_t>> update(double dt);
    const std::vector<Planet>& getBodies() const {
        return planets;
    }
};
