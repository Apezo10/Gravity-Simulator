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

PlanetSystem setup(const string& bodies) {
    istringstream input("no\n" + bodies);
    ostringstream output;
    auto* oldInput = cin.rdbuf(input.rdbuf());
    auto* oldOutput = cout.rdbuf(output.rdbuf());
    PlanetSystem system;
    bool ok = chooseSetup(system);
    cin.rdbuf(oldInput);
    cout.rdbuf(oldOutput);
    cin.clear();
    require(ok, "Setup failed");
    return system;
}

void checkSweptCollisions() {
    PlanetSystem crossing;
    crossing.setBodies({
        Planet(-10, 0, 20, 0, 1, 1),
        Planet(10, 0, -20, 0, 1, 1)
    });

    require(crossing.update(1) == vector<pair<size_t, size_t>>{{0, 1}},
        "Fast bodies passed through each other");

    const auto& merged = crossing.getBodies()[0];
    require(merged.getMass() == 2 && abs(merged.getX()) < 1e-10,
        "Swept collision did not conserve mass or center of mass");
    require(abs(merged.getXVelocity()) < 1e-10,
        "Swept collision did not conserve momentum");

    PlanetSystem diagonal;
    diagonal.setBodies({
        Planet(-10, -10, 20, 20, 1, 1),
        Planet(10, 10, -20, -20, 1, 1)
    });
    require(diagonal.update(1).size() == 1, "Diagonal crossing was missed");

    PlanetSystem nearMiss;
    nearMiss.setBodies({
        Planet(-10, 0, 20, 0, 1, 1),
        Planet(10, 3, -20, 0, 1, 1)
    });
    require(nearMiss.update(1).empty(), "Near miss incorrectly merged");

    PlanetSystem separating;
    separating.setBodies({
        Planet(-10, 0, -20, 0, 1, 1),
        Planet(10, 0, 20, 0, 1, 1)
    });
    require(separating.update(1).empty(), "Separating bodies incorrectly merged");

    // The first pair in storage collides last. Both contacts occur in one step.
    PlanetSystem chain;
    chain.setBodies({
        Planet(30, 0, 0, 0, 1, 1),
        Planet(10, 0, 0, 0, 1, 1),
        Planet(-10, 0, 100, 0, 1, 1)
    });

    require(chain.update(1) == vector<pair<size_t, size_t>>{{1, 2}, {0, 1}},
        "Multiple contacts were not resolved in time order");
    require(chain.getBodies().size() == 1 && chain.getBodies()[0].getMass() == 3,
        "Collision chain lost mass");
    require(abs(chain.getBodies()[0].getX() - 130.0 / 3) < 1e-9,
        "Merged body did not finish the remaining drift");
    require(abs(chain.getBodies()[0].getXVelocity() - 100.0 / 3) < 1e-9,
        "Collision chain lost momentum");

    // A tiny asteroid must not tunnel through a target during the real timestep.
    PlanetSystem asteroid;
    asteroid.setBodies({
        Planet(-1e7, 0, 30000, 0, 1000, 1),
        Planet(0, 0, 0, 0, 1000, 1)
    });
    require(asteroid.update(PHYSICS_STEP_SECONDS).size() == 1,
        "Asteroid tunneled through a target during a 30-minute step");
}

int main() {


    try {
        checkSweptCollisions();

        auto system = setup("2\n0\n0\n2\n-3\n2\n3\n2\n0\n-2\n5\n2\n1\n");
        auto merges = system.update(0);
        require(merges == vector<pair<size_t, size_t>>{
                {
                    0, 1}}, "Wrong merge indices");
        const auto& body = system.getBodies().at(0);
        require(system.getBodies().size() == 1 && body.getMass() == 4, "Mass not conserved");
        require(body.getX() == 0.5 && body.getY() == 0, "Wrong center of mass");
        require(body.getXVelocity() == 1 && body.getYVelocity() == -1, "Momentum not conserved");
        require(abs(pow(body.getRadius(), 3) - 16) < 1e-10, "Volume not conserved");
        system.update(10);
        require(system.getBodies()[0].getX() == 10.5, "Merged body did not move correctly");

        auto coincident = setup("3\n0\n0\n0\n0\n1\n1\n0\n0\n0\n0\n1\n1\n0\n0\n0\n0\n1\n1\n");
        require(coincident.update(1800).size() == 2, "Coincident chain not merged");
        require(coincident.getBodies().size() == 1 &&
            isfinite(coincident.getBodies()[0].getX()), "Coincident merge invalid");

        auto separate = setup("2\n0\n0\n0\n0\n1\n1\n100\n0\n0\n0\n1\n1\n");
        require(separate.update(0).empty() && separate.getBodies().size() == 2,
            "Separated bodies merged");

        auto arriving = setup("2\n0\n0\n1\n0\n1\n1\n4\n0\n-1\n0\n1\n1\n");
        require(arriving.update(1).size() == 1, "Overlap after movement not merged");
        cout << "Collision conservation, swept contacts, near misses and collision chains passed.\n";
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
