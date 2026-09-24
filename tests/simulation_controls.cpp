#include "simulation_session.hpp"
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {


    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {


    try {
        PlanetSystem initial;
        initial.setBodies({
                Planet(0, 0, 2, -3, 1, 1)});


        for (int exponent = -2; exponent <= 2; ++exponent) {
            SimulationSession session(initial);


            for (int i = 0; i < exponent; ++i) {
                session.faster();
            }


            for (int i = 0; i > exponent; --i) {
                session.slower();
            }


            for (int i = 0; i < 100; ++i) {
                session.advance(10000);
            }

            require(session.elapsedSeconds == 108000 * session.speed(), "Incorrect simulation speed");
            require(session.system.getBodies()[0].getX() == 2 * session.elapsedSeconds,
                "Speed changed physics step behavior");
        }

        SimulationSession session(initial);

        // Partial tick survives pause.
        session.advance(10000);
        session.paused = true;
        session.advance(30000000);
        require(session.elapsedSeconds == 0 && session.trails[0].count == 1, "Pause advanced state");
        session.paused = false;
        session.advance(10000);
        require(session.elapsedSeconds == 1800, "Resume lost partial tick or caught up paused time");
        session.addAsteroid(1e12, 1e12);
        session.selected = 1;
        session.following = true;
        session.faster();
        session.paused = true;
        session.reset();
        require(session.system.getBodies().size() == 1 && session.system.getBodies()[0].getX() == 0,
            "Reset did not restore initial bodies");
        require(session.trails.size() == 1 && session.trails[0].count == 1 &&
            !session.selected && !session.following && !session.paused &&
            session.speed() == 1 && session.elapsedSeconds == 0, "Reset left stale state");
        session.advance(10000);
        require(session.elapsedSeconds == 0, "Reset retained fractional time");


        for (int i = 0; i < 20; ++i) {
            session.faster();
        }

        require(session.speed() == 4, "Upper speed limit failed");


        for (int i = 0; i < 20; ++i) {
            session.slower();
        }

        require(session.speed() == 0.25, "Lower speed limit failed");

        PlanetSystem colliding;
        colliding.setBodies({Planet(0, 0, 0, 0, 1, 1), Planet(0, 0, 0, 0, 1, 1),
                Planet(1e12, 0, 0, 0, 1, 1)});
        SimulationSession merging(colliding);
        merging.selected = 1;
        merging.following = true;
        merging.advance(20000);
        require(merging.selected == 0 && merging.following && merging.trails.size() == 2,
            "Selection did not follow merged body");
        merging.reset();
        merging.selected = 2;
        merging.advance(20000);
        require(merging.selected == 1, "Selection index did not track removal");
        std::cout << "Speed, pause, reset, trail and selection controls passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
