#pragma once
#include "planet_system.hpp"
#include <array>
#include <cmath>
#include <sstream>
#include <string>

// Keep a draft separate from the live system until every field is valid.
struct CustomSetup {
    std::array<std::string, 6> fields;
    std::vector<Planet> bodies;
    std::string error;
    std::size_t activeField = 0;

    bool hasDraft() const {
        for (const auto& field : fields) if (!field.empty()) return true;
        return false;
    }

    bool addBody() {
        if (bodies.size() >= 1000) {
            error = "Limit reached: 1000 bodies.";
            return false;
        }
        std::array<double, 6> values{};
        for (std::size_t i = 0; i < fields.size(); ++i) {
            std::istringstream input(fields[i]);
            if (!(input >> values[i]) || !std::isfinite(values[i]) ||
                (i >= 4 && values[i] <= 0)) {
                activeField = i;
                error = i >= 4 ? "Radius and mass must be positive finite numbers."
                               : "Enter a finite number in each position and velocity field.";
                return false;
            }
            input >> std::ws;
            if (!input.eof()) {
                activeField = i;
                error = "Use one number per field (scientific notation is supported).";
                return false;
            }
        }
        bodies.emplace_back(values[0], values[1], values[2], values[3], values[4], values[5]);
        fields.fill("");
        activeField = 0;
        error.clear();
        return true;
    }

    bool finish(PlanetSystem& system) {
        if (hasDraft() && !addBody()) return false;
        if (bodies.empty()) {
            error = "Add at least one body to start.";
            return false;
        }
        system.setBodies(bodies);
        return true;
    }
};
