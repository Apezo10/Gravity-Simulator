#pragma once
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include "planet.hpp"
#include <vector>
inline constexpr double SCALE = 2.0e9;
struct Camera {
    double x = 0, y = 0, zoom = 1;
    sf::Vector2u viewport{800, 600};
    bool dragging = false;
    sf::Vector2f lastMouse;

    void resize(sf::Vector2u size) {
        if (size.x == 0 || size.y == 0) return;
        viewport = size;
        dragging = false;
    }

    void reset() {
        x = y = 0;
        zoom = 1;
        dragging = false;
    }

    bool fitBodies(const std::vector<Planet>& bodies) {
        double minX = 0, maxX = 0, minY = 0, maxY = 0;
        bool found = false;
        for (const auto& body : bodies) {
            const double px = body.getX(), py = body.getY();
            if (!std::isfinite(px) || !std::isfinite(py)) continue;
            if (!found) {
                minX = maxX = px;
                minY = maxY = py;
                found = true;
            } else {
                minX = std::min(minX, px);
                maxX = std::max(maxX, px);
                minY = std::min(minY, py);
                maxY = std::max(maxY, py);
            }
        }
        if (!found) return false;

        // Markers have fixed pixel radii, so reserve pixel padding, not physical radii.
        const double paddingX = std::min(50.0, viewport.x * 0.1);
        const double paddingY = std::min(50.0, viewport.y * 0.1);
        const double width = std::max(1.0, viewport.x - 2 * paddingX);
        const double height = std::max(1.0, viewport.y - 2 * paddingY);
        // Scale before subtraction to avoid overflow for widely separated bodies.
        const double spanX = maxX / SCALE - minX / SCALE;
        const double spanY = maxY / SCALE - minY / SCALE;
        x = minX * 0.5 + maxX * 0.5;
        y = minY * 0.5 + maxY * 0.5;
        if (spanX > 0 || spanY > 0) {
            zoom = std::min(spanX > 0 ? width / spanX : 10000.0,
                            spanY > 0 ? height / spanY : 10000.0);
            zoom = std::min(zoom, 10000.0);
        } else {
            zoom = 1; // A single body or coincident bodies have no extent to fit.
        }
        dragging = false;
        return true;
    }

    sf::Vector2f toScreen(double px, double py) const {
        const auto project = [this](double position, double center) {
            const double delta = position - center;
            return static_cast<float>((std::isfinite(delta) ? delta / SCALE
                : position / SCALE - center / SCALE) * zoom);
        };
        return {viewport.x * 0.5f + project(px, x),
            viewport.y * 0.5f - project(py, y)};
    }

    sf::Vector2<double> toWorld(sf::Vector2f mouse) const {
        return {x + (mouse.x - viewport.x * 0.5) * SCALE / zoom,
            y - (mouse.y - viewport.y * 0.5) * SCALE / zoom};
    }

    void drag(sf::Vector2f mouse) {
        x -= (mouse.x - lastMouse.x) * SCALE / zoom;
        y += (mouse.y - lastMouse.y) * SCALE / zoom;
        lastMouse = mouse;
    }

    void scroll(float delta, sf::Vector2f mouse) {
        double oldScale = SCALE / zoom;
        zoom = std::clamp(zoom * std::pow(1.25, delta), 1.0e-300, 10000.0);
        double newScale = SCALE / zoom;

        // Keep the world point under the cursor stationary while zooming.
        x += (mouse.x - viewport.x * 0.5) * (oldScale - newScale);
        y -= (mouse.y - viewport.y * 0.5) * (oldScale - newScale);
    }
};
