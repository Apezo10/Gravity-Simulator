#include "planet_system.hpp"
#include <algorithm>
#include <cmath>
#include <random>

using namespace std;

namespace {

    Planet mergeBodies(const Planet& a, const Planet& b) {
        const double mass = a.getMass() + b.getMass();
        const double firstMassFraction = a.getMass() / mass;
        const double secondMassFraction = b.getMass() / mass;
        const bool blackHole = a.isBlackHole() || b.isBlackHole();

        // Preserve volume for ordinary bodies; use the horizon for black holes.
        const double scale = max(a.getRadius(), b.getRadius());
        const double firstScaledRadius = a.getRadius() / scale;
        const double secondScaledRadius = b.getRadius() / scale;
        const double radius = blackHole ? 2 * G * mass / (299792458.0 * 299792458.0)
        : scale * cbrt(firstScaledRadius * firstScaledRadius * firstScaledRadius + secondScaledRadius * secondScaledRadius * secondScaledRadius);
        const Planet* appearance = &a;


        if (!a.isBlackHole() && (b.isBlackHole() || b.getMass() > a.getMass())) {
            appearance = &b;
        }

        return Planet(a.getX() * firstMassFraction + b.getX() * secondMassFraction,
            a.getY() * firstMassFraction + b.getY() * secondMassFraction,
            a.getXVelocity() * firstMassFraction + b.getXVelocity() * secondMassFraction,
            a.getYVelocity() * firstMassFraction + b.getYVelocity() * secondMassFraction,
            radius, mass, appearance->getColor(), blackHole,
            appearance->getName());

    }

}

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
                    a.getRadius() + b.getRadius()) {
                    continue;
                }

                planets[i] = mergeBodies(a, b);
                planets.erase(planets.begin() + j);
                merges.emplace_back(i, j);
                merged = true;
                break;
            }
        }
    } while (merged);
}

void PlanetSystem::calculateAccelerations() {


    for (Planet& planet : planets) {
        planet.setAcceleration(0.0, 0.0);
    }


    // Evaluate each pair once, applying gravity in opposite directions.
    for (size_t i = 0; i < planets.size(); ++i) {


        for (size_t j = i + 1; j < planets.size(); ++j) {
            Planet& a = planets[i];
            Planet& b = planets[j];
            const double dx = b.getX() - a.getX();
            const double dy = b.getY() - a.getY();
            const double distance = hypot(dx, dy);


            if (distance == 0.0) {
                continue;
            }

            const double gravity = G / distance / distance;
            const double gx = gravity * (dx / distance);
            const double gy = gravity * (dy / distance);

            // Acceleration depends on the other body's mass, not its own.
            a.addAcceleration(gx * b.getMass(), gy * b.getMass());
            b.addAcceleration(-gx * a.getMass(), -gy * a.getMass());
        }
    }
}

vector<pair<size_t, size_t>> PlanetSystem::update(double dt) {
    vector<pair<size_t, size_t>> merges;
    mergeOverlaps(merges);

    // Velocity Verlet updates velocity in two half-steps around the position step.
    calculateAccelerations();


    for (Planet& planet : planets) {
        planet.advanceVelocity(dt * 0.5);
    }


    for (Planet& planet : planets) {
        planet.advancePosition(dt);
    }

    // Merge before recalculating gravity to avoid forces inside overlapping bodies.
    // Mass-weighted half-step velocities preserve momentum across each merge.
    mergeOverlaps(merges);
    calculateAccelerations();


    for (Planet& planet : planets) {
        planet.advanceVelocity(dt * 0.5);
    }

    return merges;
}

void PlanetSystem::addAsteroid(double x, double y) {

    // Seed once, then choose new X/Y velocities (m/s) for each click.
    static mt19937 generator(random_device{
        }());
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
