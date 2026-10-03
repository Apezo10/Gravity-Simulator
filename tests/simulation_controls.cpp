#include "simulation_session.hpp"
#include "performance_stats.hpp"
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

        // Aiming freezes time without changing the user's pause setting.
        session.beginLaunch(1e12, 2e12);
        session.aimLaunch(100, -50);
        session.advance(1000000);
        require(session.elapsedSeconds == 0 && !session.paused,
            "Aiming advanced time or changed pause state");
        require(session.system.getBodies().size() == 1,
            "Aiming added a body before release");

        session.finishLaunch();
        const auto& asteroid = session.system.getBodies().back();
        require(asteroid.getX() == 1e12 && asteroid.getY() == 2e12,
            "Launch position changed");
        require(asteroid.getXVelocity() == 30000 && asteroid.getYVelocity() == 15000,
            "Drag direction or speed was incorrect");
        require(session.trails.size() == 2 && !session.launch,
            "Launch did not initialize its trail or clear aiming");
        session.finishLaunch();
        require(session.system.getBodies().size() == 2, "Release launched twice");

        session.paused = true;
        session.beginLaunch(3e12, 0);
        session.cancelLaunch();
        session.finishLaunch();
        require(session.system.getBodies().size() == 2 && session.paused,
            "Cancel launched a body or changed pause state");

        session.beginLaunch(3e12, 0);
        session.finishLaunch();
        require(session.system.getBodies().back().getXVelocity() == 0 && session.paused,
            "A click should launch at rest and preserve pause");

        session.beginLaunch(4e12, 0);
        session.reset();
        require(!session.launch, "Reset left an unfinished launch");

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
        PlanetSystem diagnosticSystem;
        diagnosticSystem.setBodies({Planet(-1, 0, 3, 0, 2, 2), Planet(1, 0, -1, 0, 2, 2)});
        SimulationSession diagnostics(diagnosticSystem);
        const auto before = measureAccuracy(diagnostics.system.getBodies());
        require(std::abs(before.energy - (10 - 2 * G)) < 1e-12 && before.momentumX == 4,
            "Energy or momentum diagnostics incorrect");
        diagnostics.advance(20000);
        require(diagnostics.system.getCollisionKineticLoss() == 8,
            "Inelastic collision kinetic loss incorrect");
        require(diagnostics.getAccuracyBaseline().energy == measureAccuracy(diagnostics.system.getBodies()).energy,
            "Merge energy jump was counted as numerical drift");
        diagnostics.addAsteroid(1e12, 0, 100, 0);
        require(diagnostics.getAccuracyBaseline().energy == measureAccuracy(diagnostics.system.getBodies()).energy,
            "Launch energy was counted as numerical drift");
        diagnostics.reset();
        require(diagnostics.system.getCollisionKineticLoss() == 0,
            "Reset retained collision energy loss");
        require(measureAccuracy({}).energy == 0 && measureAccuracy({}).energyScale == 0,
            "Empty system diagnostics invalid");
        require(!measureAccuracy({Planet(0, 0, 0, 0, 1, 1), Planet(0, 0, 0, 0, 1, 1)}).energyDefined,
            "Coincident point-mass energy was reported as defined");

        PerformanceStats stats;
        stats.add({250, 2, 4, 6, 8});
        require(stats.fps == 0, "Performance averages published before window completed");
        stats.add({250, 4, 6, 8, 10});
        require(stats.fps == 4 && stats.average.frameMs == 250 &&
            stats.average.physicsMs == 3 && stats.average.gridMs == 5 &&
            stats.average.trailsMs == 7 && stats.average.previewMs == 9,
            "Performance averages or FPS incorrect");
        stats.add({500, 1, 0, 0, 0});
        require(stats.fps == 2 && stats.average.physicsMs == 1 && stats.average.gridMs == 0,
            "Performance window retained previous samples");
        std::cout << "Speed, pause, reset, trail, selection and performance checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
