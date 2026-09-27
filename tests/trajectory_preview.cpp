#include "trajectory_preview.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        PlanetSystem system;
        system.setBodies({Planet(0, 0, 10, -5, 1, 1)});

        TrajectoryPreview preview;
        preview.calculate(system);

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
        preview.calculate(system);

        require(preview.paths[0].data() == storage, "Forecast reallocated path storage");
        require(preview.paths[0].size() == pointCount, "Forecast appended to the old path");
        require(std::abs(preview.paths[0].back().x + 20 * duration) < 1,
            "Forecast retained the previous launch velocity");

        system.setBodies({Planet(0, 0, 0, 0, 10, 1), Planet(1, 0, 0, 0, 10, 1)});
        preview.calculate(system);
        require(preview.paths[0].empty() && preview.paths[1].empty(),
            "Forecast should stop at a collision");
        require(system.getBodies().size() == 2, "Forecast merged real bodies");

        system.setBodies({});
        preview.calculate(system);
        require(preview.paths.empty(), "Empty systems should have no paths");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
