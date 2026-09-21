#pragma once
#include "camera.hpp"
#include "orbit_trail.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class Renderer {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    explicit Renderer(const sf::Font& font);
    ~Renderer();
    void draw(sf::RenderWindow& window, const std::vector<Planet>& bodies,
              const std::vector<OrbitTrail>& trails, const Camera& camera);
};
