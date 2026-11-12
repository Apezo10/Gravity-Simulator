#include "simulation_save.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    // CTest runs in the build directory; keep all fixtures isolated there.
    const auto directory = std::filesystem::current_path() / "simulation-save-fixtures";
    try {
        std::filesystem::create_directories(directory);
        const auto path = directory / "scene.json";
        PlanetSystem initial;
        initial.setBodies({Planet(-1.2345678901234567e11, 2e12, 3.141592653589793, -4000,
            1234, 1e25, BodyColor(1, 128, 255), true, "Black hole \"A\"\n\\"),
            Planet(5e12, -6e12, 7, 8, 10, 11, BodyColor(), false, ""),
            Planet(9e12, 0, 0, 0, 10, 12, BodyColor(), false, "Asteroid 1")});
        SimulationSession source(initial);
        source.elapsedSeconds = 123456.789;
        source.faster();
        source.setLaunchProperties(5.972e24, 6.371e6);
        source.beginLaunch(1, 2); // Pending launches are not committed bodies.
        std::string error;
        require(saveSimulation(source, path, error), "Save failed");

        PlanetSystem empty;
        SimulationSession loaded(empty);
        loaded.selected = 99;
        loaded.following = true;
        loaded.beginLaunch(3, 4);
        const auto revision = loaded.stateRevision();
        require(loadSimulation(loaded, path, error), "Load failed");
        require(loaded.paused && !loaded.launch && !loaded.selected && !loaded.following &&
            loaded.stateRevision() > revision, "Load left stale session state");
        require(loaded.elapsedSeconds == source.elapsedSeconds && loaded.speed() == source.speed() &&
            loaded.asteroidMass() == source.asteroidMass() &&
            loaded.asteroidRadius() == source.asteroidRadius(), "Session settings did not round trip");
        require(loaded.system.getBodies().size() == 3 && loaded.trails.size() == 3,
            "Body or trail count did not round trip");
        for (std::size_t i = 0; i < 3; ++i) {
            const auto& a = source.system.getBodies()[i];
            const auto& b = loaded.system.getBodies()[i];
            require(a.getX() == b.getX() && a.getY() == b.getY() &&
                a.getXVelocity() == b.getXVelocity() && a.getYVelocity() == b.getYVelocity() &&
                a.getMass() == b.getMass() && a.getRadius() == b.getRadius() &&
                a.storedName() == b.storedName() && a.isBlackHole() == b.isBlackHole() &&
                a.getColor().r == b.getColor().r && a.getColor().g == b.getColor().g &&
                a.getColor().b == b.getColor().b, "Body properties lost precision or metadata");
            require(loaded.trails[i].count == 1, "Load did not seed trails");
        }
        loaded.addAsteroid(2e13, 0);
        require(loaded.system.getBodies().back().storedName() == "Asteroid 2", "Loaded asteroid name reused");
        loaded.reset();
        require(loaded.system.getBodies().size() == 3, "Reset did not use loaded scene");

        // Saving twice must replace an existing file, including on Windows.
        source.elapsedSeconds = 42;
        require(saveSimulation(source, path, error) && loadSimulation(loaded, path, error) &&
            loaded.elapsedSeconds == 42, "Existing save could not be replaced");
        std::ifstream saved(path, std::ios::binary);
        const std::string valid((std::istreambuf_iterator<char>(saved)), {});
        saved.close();

        // Bad physics must not replace a good save.
        source.system.setBodies({Planet(0, 0, 0, 0, -1, 1)});
        require(!saveSimulation(source, path, error) && !error.empty(), "Invalid scene was saved");
        require(loadSimulation(loaded, path, error) && loaded.elapsedSeconds == 42,
            "Failed save destroyed previous file");

        const auto assertRejected = [&](std::string contents) {
            const auto bad = directory / "bad.json";
            { std::ofstream output(bad); output << contents; }
            const auto before = loaded.stateRevision();
            loaded.beginLaunch(10, 20);
            const auto launchRevision = loaded.stateRevision();
            require(!loadSimulation(loaded, bad, error) && !error.empty(), "Invalid file accepted");
            require(loaded.stateRevision() == launchRevision && loaded.launch &&
                loaded.elapsedSeconds == 42 && loaded.system.getBodies().size() == 3 &&
                launchRevision > before, "Failed load changed live scene");
        };
        assertRejected("{broken");
        assertRejected(valid + " trailing data");
        for (const auto& replacement : {
            std::pair<std::string, std::string>{"\"version\": 1", "\"version\": 99"},
            {"\"mass\": 1e+25", "\"mass\": -1"},
            {"\"elapsed_seconds\": 42.0", "\"elapsed_seconds\": -1"},
            {"\"speed_quarters\": 8", "\"speed_quarters\": 3"},
            {"\"black_hole\": true", "\"black_hole\": 1"},
            {"\"radius\": 1234.0", "\"radius\": 1e999"}}) {
            auto corrupted = valid;
            const auto position = corrupted.find(replacement.first);
            require(position != std::string::npos, "Corruption fixture did not match JSON output");
            corrupted.replace(position, replacement.first.size(), replacement.second);
            assertRejected(corrupted);
        }
        require(!loadSimulation(loaded, directory / "missing.json", error), "Missing file accepted");
        require(!saveSimulation(loaded, directory / "missing" / "scene.json", error),
            "Unwritable destination reported success");
        SimulationSession blank(empty);
        require(saveSimulation(blank, path, error) && loadSimulation(loaded, path, error) &&
            loaded.system.getBodies().empty() && loaded.trails.empty(), "Empty scene did not round trip");
        // Only remove known fixture files, never recursively delete a computed directory.
        std::filesystem::remove(path);
        std::filesystem::remove(directory / "bad.json");
        std::filesystem::remove(directory);
        std::cout << "Save/load round trips, replacement, validation and session restoration passed.\n";
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
