#include "trajectory_preview.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void finish(TrajectoryPreview& preview) {
    for (int frame = 0; frame < TrajectoryPreview::steps && !preview.isComplete(); ++frame) {
        preview.advance(std::chrono::seconds(1), 37);
    }

    require(preview.isComplete(), "Forecast did not finish");
}

int main() {
    try {
        PlanetSystem system;
        system.setBodies({Planet(0, 0, 10, -5, 1, 1)});

        TrajectoryPreview preview;
        preview.begin(system);
        require(!preview.isComplete() && preview.paths[0].empty(),
            "Starting a forecast should not simulate it");

        preview.advance(std::chrono::microseconds(0));
        require(preview.paths[0].empty(), "Zero time budget advanced the forecast");

        preview.advance(std::chrono::seconds(1), 12);
        require(!preview.isComplete() && preview.paths[0].size() == 1,
            "Forecast did not yield after its step limit");
        require(preview.paths[0][0].x == 10 * 12 * PHYSICS_STEP_SECONDS,
            "Incorrect incremental sample");

        finish(preview);

        const auto& end = preview.paths.at(0).back();
        const double duration = TrajectoryPreview::days * 86400.0;
        require(std::abs(end.x - 10 * duration) < 1, "Incorrect forecast X");
        require(std::abs(end.y + 5 * duration) < 1, "Incorrect forecast Y");
        require(system.getBodies()[0].getX() == 0, "Forecast moved the real body");
        require(system.getBodies()[0].getXVelocity() == 10, "Forecast changed real velocity");

        // Repeated aiming should replace old points while retaining their storage.
        const auto* storage = preview.paths[0].data();
        const auto pointCount = preview.paths[0].size();
        system.setBodies({Planet(0, 0, -20, 0, 1, 1)});
        preview.begin(system);
        preview.advance(std::chrono::seconds(1), 24);

        // Replacing unfinished work must discard its samples and simulated state.
        preview.begin(system);
        require(preview.paths[0].empty(), "Restart retained unfinished samples");
        finish(preview);

        require(preview.paths[0].data() == storage, "Forecast reallocated path storage");
        require(preview.paths[0].size() == pointCount, "Forecast appended to the old path");
        require(std::abs(preview.paths[0].back().x + 20 * duration) < 1,
            "Forecast retained the previous launch velocity");

        preview.advance();
        require(preview.paths[0].size() == pointCount, "Completed forecast kept advancing");

        // Chunk boundaries must not change gravity or the sampled trajectory.
        system.setBodies({Planet(0, 0, 0, 0, 1, 1e25),
            Planet(1e10, 0, 0, 250, 1, 1e12)});
        auto reference = system;
        preview.begin(system);
        finish(preview);

        for (int step = 1; step <= TrajectoryPreview::steps; ++step) {
            require(reference.update(PHYSICS_STEP_SECONDS).empty(), "Reference collided");

            if (step % 12 != 0) {
                continue;
            }

            for (std::size_t i = 0; i < reference.getBodies().size(); ++i) {
                const auto& body = reference.getBodies()[i];
                const auto& point = preview.paths[i].at(step / 12 - 1);
                require(point.x == body.getX() && point.y == body.getY(),
                    "Incremental forecast differs from uninterrupted physics");
            }
        }

        system.setBodies({Planet(0, 0, 0, 0, 10, 1), Planet(1, 0, 0, 0, 10, 1)});
        preview.begin(system);
        finish(preview);
        require(preview.paths[0].empty() && preview.paths[1].empty(),
            "Forecast should stop at a collision");
        require(system.getBodies().size() == 2, "Forecast merged real bodies");

        system.setBodies({});
        preview.begin(system);
        require(preview.isComplete() && preview.paths.empty(),
            "Empty systems should finish immediately without paths");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
