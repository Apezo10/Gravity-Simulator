#include "planet_system.hpp"
#include <algorithm>
#include <cmath>
#include <random>
using namespace std;

void PlanetSystem::mergeOverlaps(vector<pair<size_t, size_t>>& merges) {
    // Restart after each merge: the new radius can overlap an earlier body.
    bool merged;
    do {
        merged = false;
        for (size_t i = 0; i < planets.size() && !merged; ++i) {
            for (size_t j = i + 1; j < planets.size(); ++j) {
                const Planet& a = planets[i];
                const Planet& b = planets[j];
                if (hypot(b.getX() - a.getX(), b.getY() - a.getY()) >
                    a.getRad() + b.getRad()) continue;

                const double mass = a.getMass() + b.getMass();
                const double wa = a.getMass() / mass;
                const double wb = b.getMass() / mass;
                const bool blackHole = a.isBlackHole() || b.isBlackHole();
                // Preserve volume for ordinary bodies; use the horizon for black holes.
                const double scale = max(a.getRad(), b.getRad());
                const double ra = a.getRad() / scale;
                const double rb = b.getRad() / scale;
                const double radius = blackHole ? 2 * G * mass / (299792458.0 * 299792458.0)
                    : scale * cbrt(ra * ra * ra + rb * rb * rb);
                const Planet& appearance = a.isBlackHole() ? a : b.isBlackHole() ? b
                    : a.getMass() >= b.getMass() ? a : b;
                Planet result(a.getX() * wa + b.getX() * wb,
                              a.getY() * wa + b.getY() * wb,
                              a.getXvel() * wa + b.getXvel() * wb,
                              a.getYvel() * wa + b.getYvel() * wb,
                              radius, mass, appearance.getColor(), blackHole,
                              appearance.getName());
                planets[i] = result;
                planets.erase(planets.begin() + j);
                merges.emplace_back(i, j);
                merged = true;
                break;
            }
        }
    } while (merged);
}

void PlanetSystem::calculateAccelerations() {
    for (Planet& planet : planets) planet.setAccel(0.0, 0.0);

    // Evaluate each pair once, applying gravity in opposite directions.
    for (size_t i = 0; i < planets.size(); ++i) {
        for (size_t j = i + 1; j < planets.size(); ++j) {
            Planet& a = planets[i];
            Planet& b = planets[j];
            const double dx = b.getX() - a.getX();
            const double dy = b.getY() - a.getY();
            const double distance = hypot(dx, dy);
            if (distance == 0.0) continue;

            const double gravity = G / distance / distance;
            const double gx = gravity * (dx / distance);
            const double gy = gravity * (dy / distance);
            // Acceleration depends on the other body's mass, not its own.
            a.addAccel(gx * b.getMass(), gy * b.getMass());
            b.addAccel(-gx * a.getMass(), -gy * a.getMass());
        }
    }
}

vector<pair<size_t, size_t>> PlanetSystem::update(double dt) {
    vector<pair<size_t, size_t>> merges;
    mergeOverlaps(merges);
    // Velocity Verlet: half kick, full drift, recompute gravity, half kick.
    calculateAccelerations();
    for (Planet& planet : planets) planet.kick(dt * 0.5);
    for (Planet& planet : planets) planet.drift(dt);
    // Merge before recalculating gravity to avoid forces inside overlapping bodies.
    // Mass-weighted half-step velocities preserve momentum across each merge.
    mergeOverlaps(merges);
    calculateAccelerations();
    for (Planet& planet : planets) planet.kick(dt * 0.5);
    return merges;
}

void PlanetSystem::addAsteroid(double x, double y) {
    // Seed once, then choose new X/Y velocities (m/s) for each click.
    static mt19937 generator(random_device{}());
    uniform_real_distribution<double> velocity(-30000.0, 30000.0);

    double vx = velocity(generator);
    double vy = velocity(generator);
    double radius = 1000.0;
    double mass = 1.0e12;

    asteroidCount++;
    string name = "Asteroid " + to_string(asteroidCount);
    BodyColor color(255, 165, 0);

    // The existing drawing code keeps small bodies visible at a 3-pixel radius.
    Planet asteroid(x, y, vx, vy, radius, mass, color, false, name);
    planets.push_back(asteroid);
}
