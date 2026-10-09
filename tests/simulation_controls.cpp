#include "simulation_session.hpp"
#include "performance_stats.hpp"
#include "trajectory_preview.hpp"
#include <limits>
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

        {
            SimulationSession configured(initial);
            require(configured.asteroidMass() == 1e12 && configured.asteroidRadius() == 1000,
                "Default launch properties changed");
            configured.setLaunchProperties(5.972e24, 6.371e6);
            configured.beginLaunch(1e12, 2e12);
            configured.aimLaunch(100, -50);
            const auto revision = configured.stateRevision();
            configured.setLaunchProperties(1e26, 1e7);
            require(configured.stateRevision() > revision &&
                configured.launch->mass == 1e26 && configured.launch->radius == 1e7,
                "Aiming property change did not invalidate the forecast");
            require(configured.launch->vx == 30000 && configured.launch->vy == 15000,
                "Changing properties altered aim");
            const auto unchangedRevision = configured.stateRevision();
            configured.setLaunchProperties(0, 1);
            configured.setLaunchProperties(1, -1);
            configured.setLaunchProperties(std::numeric_limits<double>::infinity(), 1);
            configured.setLaunchProperties(1, std::numeric_limits<double>::quiet_NaN());
            require(configured.stateRevision() == unchangedRevision,
                "Invalid properties changed launch state");

            PlanetSystem forecastSystem = configured.system;
            configured.launch->addTo(forecastSystem);
            TrajectoryPreview preview;
            preview.begin(forecastSystem);
            configured.finishLaunch();
            const auto& body = configured.system.getBodies().back();
            require(body.getMass() == 1e26 && body.getRadius() == 1e7,
                "Launched body ignored configured properties");
            require(configured.trails.size() == configured.system.getBodies().size(),
                "Configured launch lost trail alignment");
            preview.advance(std::chrono::seconds(1), 12);
            require(preview.paths.back().size() == 1, "Configured forecast did not generate a sample");
            for (int i = 0; i < 12; ++i) configured.system.update(PHYSICS_STEP_SECONDS);
            const auto& actual = configured.system.getBodies().back();
            require(preview.paths.back()[0].x == actual.getX() &&
                preview.paths.back()[0].y == actual.getY(),
                "Configured launch diverged from its forecast");

            configured.beginLaunch(3e12, 0);
            require(configured.launch->mass == 1e26 && configured.launch->radius == 1e7,
                "Next launch lost settings");
            configured.cancelLaunch();
            configured.reset();
            require(configured.asteroidMass() == 1e26 && configured.asteroidRadius() == 1e7,
                "Reset discarded launch preferences");
            configured.setLaunchProperties(1e100, 1e100);
            require(configured.asteroidMass() == 1e30 && configured.asteroidRadius() == 1e9,
                "Upper launch bounds failed");
            configured.setLaunchProperties(1e-100, 1e-100);
            require(configured.asteroidMass() == 1 && configured.asteroidRadius() == 1,
                "Lower launch bounds failed");

            // A larger physical radius must change collision behavior, not just appearance.
            PlanetSystem small, large;
            small.setBodies({Planet(0, 0, 0, 0, 1, 1)});
            large = small;
            small.addAsteroid(100, 0, 0, 0, 1, 1);
            large.addAsteroid(100, 0, 0, 0, 100, 1);
            require(small.update(0.01).empty(), "Small projectile collided unexpectedly");
            require(large.update(0.01).size() == 1, "Configured radius did not affect collisions");
        }

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
        const auto balanced = measureAccuracy({Planet(-10, 0, 3, 4, 1, 2),
            Planet(10, 0, -3, -4, 1, 2)});
        require(balanced.momentumX == 0 && balanced.momentumY == 0 && balanced.momentumScale == 20,
            "Balanced momentum normalization incorrect");
        auto perturbed = balanced;
        perturbed.momentumX = 3;
        perturbed.momentumY = 4;
        const auto drift = measureMomentumDrift(perturbed, balanced);
        require(drift.defined && drift.relativeDefined && drift.magnitude == 5 && drift.relative == 0.25,
            "Zero-net-momentum drift incorrect");
        const auto stationaryDrift = measureMomentumDrift(perturbed, measureAccuracy({}));
        require(stationaryDrift.defined && !stationaryDrift.relativeDefined,
            "Zero momentum scale produced relative drift");
        const auto extreme = measureAccuracy({Planet(0, 0, 1e200, 0, 1, 1e200)});
        require(!extreme.momentumDefined && !measureMomentumDrift(extreme, balanced).defined &&
            !measureMomentumDrift(balanced, extreme).defined,
            "Nonfinite momentum was reported as valid drift");
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
        require(measureMomentumDrift(measureAccuracy(diagnostics.system.getBodies()),
            diagnostics.getAccuracyBaseline()).magnitude == 0,
            "Launch momentum was counted as numerical drift");
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
