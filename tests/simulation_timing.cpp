#include "setup.hpp"
#include "simulation_timing.hpp"
#include "orbit_trail.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
using namespace std;
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

array<double, 5> run(int frames, bool uneven) {
    istringstream input("yes\n1\n");
    ostringstream output;
    auto* oldInput = cin.rdbuf(input.rdbuf());
    auto* oldOutput = cout.rdbuf(output.rdbuf());
    PlanetSystem system;
    bool ok = chooseSetup(system);
    cin.rdbuf(oldInput);
    cout.rdbuf(oldOutput);
    cin.clear();
    require(ok, "Preset setup failed");
    SimulationTiming timing;
    OrbitTrail trail;
    int steps = 0;
    std::int64_t previous = 0;
    for (int i = 1; i <= frames; ++i) {
        // Exactly ten seconds split into different rendering schedules.
        std::int64_t now = 10000000LL * i / frames;
        if (uneven && i < frames && i % 2) now -= 3000;
        timing.advance(now - previous, [&](double dt) {
            system.update(dt);
            if (++steps % 4 == 0) trail.add(system.getParSystem()[1]);
        });
        previous = now;
    }
    require(steps == 600, "Wrong step count for ten seconds");
    require(trail.count == 150, "Trail samples depend on frame rate");
    const auto& earth = system.getParSystem()[1];
    return {earth.getX(), earth.getY(), earth.getXvel(), earth.getYvel(), double(trail.count)};
}

int main() {
    try {
        const auto baseline = run(600, false);
        for (int fps : {15, 30, 60, 144, 240})
            for (bool uneven : {false, true})
                require(run(fps * 10, uneven) == baseline, "Orbit depends on rendering schedule");
        SimulationTiming timing;
        int steps = 0;
        auto step = [&](double dt) { require(dt == 1800, "Variable physics timestep"); ++steps; };
        timing.advance(10000, step);
        require(steps == 0, "Stepped too early");
        timing.advance(10000, step);
        require(steps == 1, "Fractional time lost");
        timing.advance(30000000, step);
        require(steps == 16, "Stall catch-up unbounded");
        timing.advance(0, step);
        timing.advance(-100, step);
        require(steps == 16, "Stall left a backlog");
        Planet body(0, 0, 2, -3, 1, 1);
        body.update(10);
        require(body.getX() == 20 && body.getY() == -30, "Supplied timestep ignored");
        cout << "Timing passed at 15, 30, 60, 144 and 240 FPS, with uneven frames and stalls.\n";
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
