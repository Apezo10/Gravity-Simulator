#pragma once
#include "camera.hpp"
#include "orbit_trail.hpp"
#include "simulation_session.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class Renderer {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    explicit Renderer(const sf::Font& font);
    ~Renderer();
    std::optional<std::size_t> pickBody(sf::Vector2f mouse,
        const std::vector<Planet>& bodies, const Camera& camera) const;
    void draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera);
};
