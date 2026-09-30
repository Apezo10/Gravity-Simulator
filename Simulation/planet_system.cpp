#include "planet_system.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

using namespace std;

namespace {

    optional<double> collisionTime(const Planet& a, const Planet& b, double dt) {
        const double dx = b.getX() - a.getX();
        const double dy = b.getY() - a.getY();
        const double vx = b.getXVelocity() - a.getXVelocity();
        const double vy = b.getYVelocity() - a.getYVelocity();
        const double speed = hypot(vx, vy);

        if (speed == 0) {
            return nullopt;
        }

        // During Verlet's position step, relative motion is a straight line.
        const double ux = vx / speed;
        const double uy = vy / speed;
        const double along = dx * ux + dy * uy;
        const double across = abs(dx * uy - dy * ux);
        const double radius = a.getRadius() + b.getRadius();

        if (along >= 0 || across > radius) {
            return nullopt;
        }

        // Find first contact without squaring astronomical distances.
        const double ratio = across / radius;
        const double chord = radius * sqrt((1 - ratio) * (1 + ratio));
        const double time = max(0.0, (-along - chord) / speed);

        if (time > dt) {
            return nullopt;
        }

        return time;
    }

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

void PlanetSystem::mergePair(size_t first, size_t second,
    vector<pair<size_t, size_t>>& merges) {
    planets[first] = mergeBodies(planets[first], planets[second]);
    planets.erase(planets.begin() + second);

    accelerationsReady = false;
    merges.emplace_back(first, second);
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

                mergePair(i, j, merges);
                merged = true;
                break;
            }
        }
    } while (merged);
}

void PlanetSystem::advancePositions(double dt, vector<pair<size_t, size_t>>& merges) {
    while (true) {
        double nextTime = dt;
        optional<pair<size_t, size_t>> nextPair;

        // Resolve the earliest contact first, regardless of body storage order.
        for (size_t i = 0; i < planets.size(); ++i) {
            for (size_t j = i + 1; j < planets.size(); ++j) {
                const auto time = collisionTime(planets[i], planets[j], nextTime);

                if (time && (!nextPair || *time < nextTime)) {
                    nextTime = *time;
                    nextPair = pair{i, j};
                }
            }
        }

        for (Planet& planet : planets) {
            planet.advancePosition(nextTime);
        }

        if (!nextPair) {
            break;
        }

        // Merge the detected pair directly: rounding can leave a tiny contact gap.
        mergePair(nextPair->first, nextPair->second, merges);
        mergeOverlaps(merges);

        // Continue the remaining drift with the merged body's momentum.
        dt -= nextTime;
    }
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

    accelerationsReady = true;
}

vector<pair<size_t, size_t>> PlanetSystem::update(double dt) {
    vector<pair<size_t, size_t>> merges;
    mergeOverlaps(merges);

    // The previous step already calculated gravity at these positions.
    if (!accelerationsReady) {
        calculateAccelerations();
    }


    // Velocity Verlet updates velocity in two half-steps around the position step.
    for (Planet& planet : planets) {
        planet.advanceVelocity(dt * 0.5);
    }


    accelerationsReady = false;

    advancePositions(dt, merges);

    // Merge before recalculating gravity to avoid forces inside overlapping bodies.
    // Mass-weighted half-step velocities preserve momentum across each merge.
    mergeOverlaps(merges);
    calculateAccelerations();


    for (Planet& planet : planets) {
        planet.advanceVelocity(dt * 0.5);
    }

    return merges;
}

void PlanetSystem::addAsteroid(double x, double y, double vx, double vy) {
    double radius = 1000.0;
    double mass = 1.0e12;

    asteroidCount++;
    string name = "Asteroid " + to_string(asteroidCount);
    BodyColor color(255, 165, 0);

    // The existing drawing code keeps small bodies visible at a 3-pixel radius.
    Planet asteroid(x, y, vx, vy, radius, mass, color, false, name);
    planets.push_back(asteroid);
    accelerationsReady = false;
}
