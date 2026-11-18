#pragma once
#include <cmath>
#include <cstddef>

// Supported setup ranges in SI units. Leave ample arithmetic headroom for
// gravitational acceleration, integration, merging and energy diagnostics.
namespace InputLimits {
    inline constexpr double maxPosition = 1e20;
    inline constexpr double maxVelocity = 1e8;
    inline constexpr double minRadius = 1e-3;
    inline constexpr double maxRadius = 1e15;
    inline constexpr double minMass = 1e-12;
    inline constexpr double maxMass = 1e40;

    inline bool valid(std::size_t field, double value) {
        if (!std::isfinite(value)) return false;
        if (field < 2) return std::abs(value) <= maxPosition;
        if (field < 4) return std::abs(value) <= maxVelocity;
        if (field == 4) return value >= minRadius && value <= maxRadius;
        return field == 5 && value >= minMass && value <= maxMass;
    }

    inline const char* error(std::size_t field) {
        if (field < 2) return "Position must be between -1e20 and 1e20 m.";
        if (field < 4) return "Velocity must be between -1e8 and 1e8 m/s.";
        if (field == 4) return "Radius must be between 1e-3 and 1e15 m.";
        return "Mass must be between 1e-12 and 1e40 kg.";
    }
}
