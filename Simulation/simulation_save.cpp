#include "simulation_save.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
using nlohmann::json;
constexpr std::uintmax_t maxFileBytes = 16 * 1024 * 1024;
constexpr std::size_t maxBodies = 10000;

double number(const json& object, const char* key, bool positive = false) {
    const auto& value = object.at(key);
    if (!value.is_number()) throw std::runtime_error("Expected a numeric field.");
    const double result = value.get<double>();
    if (!std::isfinite(result) || (positive && result <= 0))
        throw std::runtime_error("Body values must be finite; mass and radius must be positive.");
    return result;
}

struct Snapshot {
    std::vector<Planet> bodies;
    double elapsed, mass, radius;
    int quarters;
};

Snapshot decode(const json& root) {
    if (!root.at("version").is_number_integer() || root.at("version") != 1)
        throw std::runtime_error("Unsupported save version.");
    Snapshot snapshot;
    snapshot.elapsed = number(root, "elapsed_seconds");
    const double speed = number(root, "speed_quarters");
    if (snapshot.elapsed < 0 || (speed != 1 && speed != 2 && speed != 4 && speed != 8 && speed != 16))
        throw std::runtime_error("Invalid simulation time or speed.");
    snapshot.quarters = static_cast<int>(speed);
    snapshot.mass = number(root, "launch_mass", true);
    snapshot.radius = number(root, "launch_radius", true);
    if (snapshot.mass < 1 || snapshot.mass > 1e30 || snapshot.radius < 1 || snapshot.radius > 1e9)
        throw std::runtime_error("Launch properties are outside supported bounds.");
    const auto& bodies = root.at("bodies");
    if (!bodies.is_array() || bodies.size() > maxBodies)
        throw std::runtime_error("Save must contain an array of at most 10000 bodies.");
    for (const auto& body : bodies) {
        const auto& color = body.at("color");
        if (!color.is_array() || color.size() != 3)
            throw std::runtime_error("Invalid body color.");
        for (const auto& channel : color)
            if (!channel.is_number_integer() || channel < 0 || channel > 255)
                throw std::runtime_error("Invalid body color channel.");
        if (!body.at("name").is_string() || body.at("name").get_ref<const std::string&>().size() > 1024 ||
            !body.at("black_hole").is_boolean())
            throw std::runtime_error("Invalid body name or black hole flag.");
        snapshot.bodies.emplace_back(number(body, "x"), number(body, "y"),
            number(body, "vx"), number(body, "vy"), number(body, "radius", true),
            number(body, "mass", true), BodyColor(color[0].get<int>(), color[1].get<int>(), color[2].get<int>()),
            body.at("black_hole").get<bool>(), body.at("name").get<std::string>());
    }
    return snapshot;
}
}

bool saveSimulation(const SimulationSession& session, const std::filesystem::path& path,
    std::string& error) {
    auto temporary = path;
    temporary += ".tmp";
    try {
        json root = {{"version", 1}, {"elapsed_seconds", session.elapsedSeconds},
            {"speed_quarters", static_cast<int>(session.speed() * 4)},
            {"launch_mass", session.asteroidMass()}, {"launch_radius", session.asteroidRadius()},
            {"bodies", json::array()}};
        for (const auto& body : session.system.getBodies()) {
            const auto color = body.getColor();
            root["bodies"].push_back({{"name", body.storedName()}, {"x", body.getX()}, {"y", body.getY()},
                {"vx", body.getXVelocity()}, {"vy", body.getYVelocity()}, {"mass", body.getMass()},
                {"radius", body.getRadius()}, {"color", {color.r, color.g, color.b}},
                {"black_hole", body.isBlackHole()}});
        }
        decode(root); // Never replace a usable save with a scene we cannot load.
        const auto contents = root.dump(2) + '\n';
        if (contents.size() > maxFileBytes) throw std::runtime_error("Save exceeds 16 MiB.");
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.close();
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not replace save file.");
#else
        std::filesystem::rename(temporary, path);
#endif
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        error = exception.what();
        return false;
    }
}

bool loadSimulation(SimulationSession& session, const std::filesystem::path& path,
    std::string& error) {
    try {
        if (std::filesystem::file_size(path) > maxFileBytes)
            throw std::runtime_error("Save exceeds 16 MiB.");
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Could not open save file.");
        const auto snapshot = decode(json::parse(input));
        session.restore(snapshot.bodies, snapshot.elapsed, snapshot.quarters, snapshot.mass, snapshot.radius);
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}
