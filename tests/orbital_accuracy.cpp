#include "setup.hpp"
#include "simulation_timing.hpp"
#include "orbit_trail.hpp"
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

int main() {


    try {
        checkAccelerationCache();

        Planet body(0, 0, 2, -3, 1, 1);
        body.setAcceleration(4, -2);
        body.update(2);
        require(body.getX() == 12 && body.getY() == -10 &&
            body.getXVelocity() == 10 && body.getYVelocity() == -7,
            "Constant acceleration motion incorrect");

        auto system = circularSystem();
        const auto& bodies = system.getBodies();
        const double initialEnergy = energy(bodies);
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
