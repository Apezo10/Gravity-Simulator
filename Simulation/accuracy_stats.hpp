#pragma once
#include "planet_system.hpp"
#include <cmath>

struct AccuracyStats {
    double energy = 0, energyScale = 0;
    double momentumX = 0, momentumY = 0;
    bool energyDefined = true;
};

inline AccuracyStats measureAccuracy(const std::vector<Planet>& bodies) {
    AccuracyStats result;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const auto& a = bodies[i];
        const double kinetic = 0.5 * a.getMass() *
            (a.getXVelocity() * a.getXVelocity() + a.getYVelocity() * a.getYVelocity());
        result.energy += kinetic;
        result.energyScale += kinetic;
        result.momentumX += a.getMass() * a.getXVelocity();
        result.momentumY += a.getMass() * a.getYVelocity();
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            const auto& b = bodies[j];
            const double distance = std::hypot(b.getX() - a.getX(), b.getY() - a.getY());
            if (distance == 0) { result.energyDefined = false; continue; }
            const double potential = G * a.getMass() / distance * b.getMass();
            result.energy -= potential;
            result.energyScale += potential;
        }
    }
    result.energyDefined &= std::isfinite(result.energy) && std::isfinite(result.energyScale);
    return result;
}
