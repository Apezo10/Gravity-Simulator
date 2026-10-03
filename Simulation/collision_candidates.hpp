#pragma once
#include "planet.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

class CollisionCandidates {
    struct Bounds { double left, right, bottom, top; std::size_t index; };
    std::vector<Bounds> bounds;
public:
    template <typename Visit>
    void visit(const std::vector<Planet>& bodies, double dt, Visit visitPair) {
        // Small systems avoid sorting overhead; non-finite bounds use the
        // exact fallback rather than risk discarding a possible contact.
        bool fallback = bodies.size() < 32;
        bounds.clear();
        if (!fallback) {
            bounds.reserve(bodies.size());
            for (std::size_t i = 0; i < bodies.size(); ++i) {
                const auto& body = bodies[i];
                const double x = body.getX(), y = body.getY();
                const double endX = x + body.getXVelocity() * dt;
                const double endY = y + body.getYVelocity() * dt;
                const double r = body.getRadius();
                Bounds box{std::min(x, endX) - r, std::max(x, endX) + r,
                    std::min(y, endY) - r, std::max(y, endY) + r, i};
                if (!std::isfinite(box.left) || !std::isfinite(box.right) ||
                    !std::isfinite(box.bottom) || !std::isfinite(box.top)) { fallback = true; break; }
                // Expand outward for rounding at astronomical coordinates.
                const double infinity = std::numeric_limits<double>::infinity();
                box.left = std::nextafter(box.left, -infinity);
                box.right = std::nextafter(box.right, infinity);
                box.bottom = std::nextafter(box.bottom, -infinity);
                box.top = std::nextafter(box.top, infinity);
                bounds.push_back(box);
            }
        }
        if (fallback) {
            for (std::size_t i = 0; i < bodies.size(); ++i)
                for (std::size_t j = i + 1; j < bodies.size(); ++j) visitPair(i, j);
            return;
        }
        std::sort(bounds.begin(), bounds.end(), [](const Bounds& a, const Bounds& b) {
            return a.left < b.left || (a.left == b.left && a.index < b.index);
        });
        for (std::size_t i = 0; i < bounds.size(); ++i) {
            const auto& a = bounds[i];
            for (std::size_t j = i + 1; j < bounds.size(); ++j) {
                const auto& b = bounds[j];
                if (b.left > a.right) break;
                if (b.bottom > a.top || a.bottom > b.top) continue;
                visitPair(std::min(a.index, b.index), std::max(a.index, b.index));
            }
        }
    }
};
