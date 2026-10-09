#include "setup.hpp"
#include "simulation_timing.hpp"
#include "orbit_trail.hpp"
#include "accuracy_stats.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;
#include <stdexcept>

void require(bool condition, const char* message) {


    if (!condition) {
        throw runtime_error(message);
    }
}

PlanetSystem circularSystem() {
    istringstream input("yes\n1\n");
    ostringstream output;

    auto* oldInput = cin.rdbuf(input.rdbuf());
    auto* oldOutput = cout.rdbuf(output.rdbuf());
    
    PlanetSystem system;
    const bool ok = chooseSetup(system);
    
    cin.rdbuf(oldInput);
    cout.rdbuf(oldOutput);
    cin.clear();
    
    
    require(ok, "Preset setup failed");
    return system;
}

double energy(const vector<Planet>& bodies) {
    double result = 0;


    for (const auto& body : bodies)
    {
        result += 0.5 * body.getMass() *
        (body.getXVelocity() * body.getXVelocity() + body.getYVelocity() * body.getYVelocity());
    }

    return result - G * bodies[0].getMass() * bodies[1].getMass() /
    hypot(bodies[1].getX() - bodies[0].getX(), bodies[1].getY() - bodies[0].getY());
}

double orbitError(int steps) {
    auto system = circularSystem();
    const auto& initial = system.getBodies();
    const double separation = initial[1].getX() - initial[0].getX();
    const double period = 2 * acos(-1.0) * sqrt(pow(separation, 3) /
        (G * (initial[0].getMass() + initial[1].getMass())));


    for (int i = 0; i < steps; ++i) {
        system.update(period / steps);
    }

    const auto& bodies = system.getBodies();
    return hypot(bodies[1].getX() - bodies[0].getX() - separation,
        bodies[1].getY() - bodies[0].getY()) / separation;
}

void checkAccelerationCache() {
    auto cached = circularSystem();
    cached.update(1800);

    // Copies used by trajectory forecasts retain valid accelerations too.
    auto reference = cached;

    for (int step = 0; step < 100; ++step) {
        if (step == 25) {
            cached.addAsteroid(2e11, 1e11, 10000, -5000);
            reference.addAsteroid(2e11, 1e11, 10000, -5000);
        }

        if (step == 50) {
            // Reset into a collision, with a third body still exerting gravity.
            const vector<Planet> bodies = {
                Planet(0, 0, 1, 0, 1, 1e10),
                Planet(4, 0, -1, 0, 1, 1e10),
                Planet(100, 0, 0, 0, 1, 1e12)
            };
            cached.setBodies(bodies);
            reference.setBodies(bodies);
        }

        // Force the reference to recalculate at both ends of every step.
        reference.setBodies(reference.getBodies());
        const double dt = step < 50 ? (step % 2 == 0 ? 1800 : 900) : 1;
        require(cached.update(dt) == reference.update(dt), "Cache changed collision results");

        const auto& actual = cached.getBodies();
        const auto& expected = reference.getBodies();
        require(actual.size() == expected.size(), "Cache changed body count");

        for (size_t i = 0; i < actual.size(); ++i) {
            require(actual[i].getX() == expected[i].getX() &&
                actual[i].getY() == expected[i].getY() &&
                actual[i].getXVelocity() == expected[i].getXVelocity() &&
                actual[i].getYVelocity() == expected[i].getYVelocity() &&
                actual[i].getMass() == expected[i].getMass(),
                "Cached gravity differs from full recalculation");
        }
    }
}

void checkCloseEncounters() {
    const double mass = 1e24;
    const double period = 3600;
    const double separation = cbrt(G * (2 * mass) * pow(period / (2 * acos(-1.0)), 2));
    const double speed = acos(-1.0) * separation / period;
    PlanetSystem tight;
    tight.setBodies({Planet(-separation / 2, 0, 0, -speed, 1000, mass),
        Planet(separation / 2, 0, 0, speed, 1000, mass)});
    const double initialEnergy = energy(tight.getBodies());
    for (int i = 0; i < 40; ++i) {
        require(tight.update(PHYSICS_STEP_SECONDS).empty(), "Tight orbit spuriously collided");
        require(abs((energy(tight.getBodies()) - initialEnergy) / initialEnergy) < 1e-4,
            "Substeps did not preserve tight-orbit energy");
        const auto& bodies = tight.getBodies();
        require(abs(hypot(bodies[1].getX() - bodies[0].getX(),
            bodies[1].getY() - bodies[0].getY()) / separation - 1) < 0.005,
            "Tight orbit expanded despite encounter substeps");
    }

    PlanetSystem flyby;
    flyby.setBodies({Planet(0, 0, 0, 0, 1000, mass),
        Planet(-5e7, 1e7, 60000, 0, 1000, 1e12)});
    auto reference = flyby;
    require(flyby.update(PHYSICS_STEP_SECONDS).empty(), "Flyby spuriously collided");
    for (int i = 0; i < 18000; ++i) reference.update(0.1);
    const auto& actual = flyby.getBodies()[1];
    const auto& expected = reference.getBodies()[1];
    require(hypot(actual.getX() - expected.getX(), actual.getY() - expected.getY()) < 1e4,
        "Fast flyby differs too far from fine-step reference");
    require(hypot(actual.getXVelocity() - expected.getXVelocity(),
        actual.getYVelocity() - expected.getYVelocity()) < 10,
        "Fast flyby velocity differs too far from fine-step reference");
}

void checkExtremeGravityScales() {
    // Both scenes use finite, positive inputs accepted by custom setup.
    for (bool tiny : {true, false}) {
        const double distance = tiny ? 1e-200 : 1e200;
        const double mass = tiny ? 1e-300 : 1e300;
        const double radius = tiny ? 1e-210 : 1;
        const double dt = tiny ? 1e-300 : 1;
        const double expectedVelocity = tiny ? G * 1e-200 : G * 1e-100;
        PlanetSystem system;
        system.setBodies({Planet(0, 0, 0, 0, radius, mass),
            Planet(distance, 0, 0, 0, radius, mass)});
        require(system.update(dt).empty(), "Extreme-scale separated bodies merged");
        for (const auto& body : system.getBodies()) {
            require(isfinite(body.getX()) && isfinite(body.getY()) &&
                isfinite(body.getXVelocity()) && isfinite(body.getYVelocity()),
                "Representable extreme-scale gravity produced nonfinite motion");
            require(abs(abs(body.getXVelocity()) / expectedVelocity - 1) < 1e-12,
                "Representable extreme-scale gravity was lost or miscalculated");
            require(body.getYVelocity() == 0, "Axial gravity introduced sideways motion");
        }
        require(system.getBodies()[0].getXVelocity() > 0 &&
            system.getBodies()[1].getXVelocity() < 0, "Gravity is not attractive");
    }
}

void checkLargeSceneForces() {
    // Stationary bodies move by less than one coordinate ULP in one second,
    // so an independent hypot-based sum gives the expected final velocities.
    std::vector<Planet> bodies;
    for (int i = 0; i < 512; ++i) {
        bodies.emplace_back(1e12 + (i % 32) * 1e9, 1e12 + (i / 32) * 1e9,
            0, 0, 1000, 1e12 + (i % 7) * 1e11);
    }
    PlanetSystem system;
    system.setBodies(bodies);
    require(system.update(1).empty(), "Sparse large scene spuriously merged");
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        double ax = 0, ay = 0;
        for (std::size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            const double dx = bodies[j].getX() - bodies[i].getX();
            const double dy = bodies[j].getY() - bodies[i].getY();
            const double distance = std::hypot(dx, dy);
            const double acceleration = G / distance / distance * bodies[j].getMass();
            ax += acceleration * (dx / distance);
            ay += acceleration * (dy / distance);
        }
        const auto& actual = system.getBodies()[i];
        require(actual.getX() == bodies[i].getX() && actual.getY() == bodies[i].getY(),
            "Force reference assumes stationary coordinates");
        require(std::hypot(actual.getXVelocity() - ax, actual.getYVelocity() - ay) /
            std::hypot(ax, ay) < 1e-12,
            "Large-scene acceleration differs from direct Newtonian reference");
    }
}

int main() {


    try {
        checkAccelerationCache();
        checkCloseEncounters();
        checkExtremeGravityScales();
        checkLargeSceneForces();

        Planet body(0, 0, 2, -3, 1, 1);
        body.setAcceleration(4, -2);
        body.update(2);
        require(body.getX() == 12 && body.getY() == -10 &&
            body.getXVelocity() == 10 && body.getYVelocity() == -7,
            "Constant acceleration motion incorrect");

        auto system = circularSystem();
        const auto& bodies = system.getBodies();
        const double initialEnergy = energy(bodies);
        require(abs((measureAccuracy(bodies).energy - initialEnergy) / initialEnergy) < 1e-14,
            "Displayed orbital energy differs from accuracy reference");
        const double separation = bodies[1].getX() - bodies[0].getX();
        const double momentumScale = bodies[1].getMass() * abs(bodies[1].getYVelocity());
        double maxEnergyError = 0, maxRadiusError = 0;


        for (int step = 0; step < 175320; ++step) {
            require(system.update(PHYSICS_STEP_SECONDS).empty(), "Circular orbit collided");
            maxEnergyError = max(maxEnergyError, abs((energy(bodies) - initialEnergy) / initialEnergy));
            maxRadiusError = max(maxRadiusError, abs(hypot(bodies[1].getX() - bodies[0].getX(),
                bodies[1].getY() - bodies[0].getY()) / separation - 1));
            const double px = bodies[0].getMass() * bodies[0].getXVelocity() + bodies[1].getMass() * bodies[1].getXVelocity();
            const double py = bodies[0].getMass() * bodies[0].getYVelocity() + bodies[1].getMass() * bodies[1].getYVelocity();
            require(hypot(px, py) / momentumScale < 1e-10, "Momentum advancePosition too large");
        }

        require(maxEnergyError < 1e-9, "Ten-year energy error too large");
        require(maxRadiusError < 1e-6, "Ten-year orbital radius error too large");
        const double coarse = orbitError(200), fine = orbitError(400);
        require(fine < coarse / 3.5 && fine > coarse / 4.5,
            "Halving timestep did not give second-order convergence");
        cout << "Ten-year maximum relative energy error: " << maxEnergyError
        << "; radius error: " << maxRadiusError
        << "; convergence ratio: " << coarse / fine << '\n';
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
