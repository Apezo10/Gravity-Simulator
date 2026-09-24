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

int main() {


    try {
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
        cout << "Collision conservation, coincident chains, separation and movement passed.\n";
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
