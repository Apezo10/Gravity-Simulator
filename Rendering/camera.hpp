#pragma once
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
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

    sf::Vector2f toScreen(double px, double py) const {
        return {viewport.x * 0.5f + static_cast<float>((px - x) * zoom / SCALE),
            viewport.y * 0.5f - static_cast<float>((py - y) * zoom / SCALE)};
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
        zoom = std::clamp(zoom * std::pow(1.25, delta), 0.01, 10000.0);
        double newScale = SCALE / zoom;

        // Keep the world point under the cursor stationary while zooming.
        x += (mouse.x - viewport.x * 0.5) * (oldScale - newScale);
        y -= (mouse.y - viewport.y * 0.5) * (oldScale - newScale);
    }
};
