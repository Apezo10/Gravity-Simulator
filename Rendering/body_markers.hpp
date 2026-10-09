#pragma once
#include "planet.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <array>

inline std::size_t bodyMarkerIndex(const Planet& body) {
    const double mass = body.getMass();
    if (mass > 1e29) return 4;
    if (mass > 1e26) return 3;
    if (mass > 1e24) return 2;
    if (mass > 1e23) return 1;
    return 0;
}

inline float displayRadius(const Planet& body) {
    constexpr std::array<float, 5> radii{3, 6, 10, 16, 30};
    return radii[bodyMarkerIndex(body)];
}

class BodyMarkers {
    static sf::CircleShape circle(float radius, float outline) {
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        if (outline != 0) shape.setOutlineThickness(outline);
        return shape;
    }
    std::array<sf::CircleShape, 5> ordinary{
        circle(3, 0), circle(6, 0), circle(10, 0), circle(16, 0), circle(30, 0)};
    std::array<sf::CircleShape, 5> blackHoles{
        circle(3, 2), circle(6, 2), circle(10, 2), circle(16, 2), circle(30, 2)};
    std::array<sf::CircleShape, 5> selected{
        circle(8, 1.5f), circle(11, 1.5f), circle(15, 1.5f), circle(21, 1.5f), circle(35, 1.5f)};
public:
    sf::CircleShape& body(const Planet& planet) {
        return (planet.isBlackHole() ? blackHoles : ordinary)[bodyMarkerIndex(planet)];
    }
    sf::CircleShape& selection(const Planet& planet) {
        return selected[bodyMarkerIndex(planet)];
    }
};
