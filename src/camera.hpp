#pragma once
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
inline constexpr double SCALE = 2.0e9;
struct Camera {
    double x = 0, y = 0, zoom = 1;
    bool dragging = false;
    sf::Vector2f lastMouse;

    sf::Vector2f toScreen(double px, double py) const {
        return {400.0f + static_cast<float>((px - x) * zoom / SCALE),
            300.0f - static_cast<float>((py - y) * zoom / SCALE)};
    }

    sf::Vector2<double> toWorld(sf::Vector2f mouse) const {
        return {x + (mouse.x - 400.0) * SCALE / zoom,
            y - (mouse.y - 300.0) * SCALE / zoom};
    }

    void drag(sf::Vector2f mouse) {
        x -= (mouse.x - lastMouse.x) * SCALE / zoom;
        y += (mouse.y - lastMouse.y) * SCALE / zoom;
        lastMouse = mouse;
    }

    void scroll(float delta, sf::Vector2f mouse) {
        double oldScale = SCALE / zoom;
        zoom = std::clamp(zoom * std::pow(1.25, delta), 0.01, 10000.0);
        double newScale = SCALE / zoom;

        // Keep the world point under the cursor stationary while zooming.
        x += (mouse.x - 400) * (oldScale - newScale);
        y -= (mouse.y - 300) * (oldScale - newScale);
    }
};
