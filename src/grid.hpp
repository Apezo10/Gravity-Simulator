#pragma once

#include "camera.hpp"
#include "planet.hpp"
#include <SFML/Graphics/Vertex.hpp>
#include <vector>

class Grid {

private:
    std::vector<std::vector<sf::Vertex>> verticalLines;
    std::vector<std::vector<sf::Vertex>> horizontalLines;

    struct GravityWell {
        double x = 0, y = 0;
        double mass = -1, depth = 0;
        bool blackHole = false;
    };

    std::vector<GravityWell> wells;
    Camera lastCamera;
    bool ready = false;

    sf::Vector2f distortPoints(float x, float y, const Camera& camera) const {

        // Convert the screen sample to world metres before doing any physics-like math.
        const auto world = camera.toWorld({x, y});
        const double worldX = world.x;
        const double worldY = world.y;
        double dxTotal = 0.0;
        double dyTotal = 0.0;

        for (const auto& well : wells) {

            const double dx = well.x - worldX;
            const double dy = well.y - worldY;
            const double distance = std::max(std::hypot(dx, dy), 10.0 * SCALE);

            const double strength = std::min(well.depth / (1.0 + distance / (80.0 * SCALE)),
                0.9 * distance);

            dxTotal += strength * dx / distance;
            dyTotal += strength * dy / distance;
        }

        const double displacement = std::hypot(dxTotal, dyTotal);
        const double limit = 150.0 * SCALE;

        if (displacement > limit) {
            dxTotal *= limit / displacement;
            dyTotal *= limit / displacement;
        }

        // Project the distorted world position back through the camera.
        return camera.toScreen(worldX + dxTotal, worldY + dyTotal);
    }

public:
    Grid()
        : verticalLines(16, std::vector<sf::Vertex>(120)),
          horizontalLines(12, std::vector<sf::Vertex>(160)) {}

    bool updateGrid(const std::vector<Planet>& planets, const Camera& camera) {
        bool changed = !ready || wells.size() != planets.size()
            || camera.x != lastCamera.x || camera.y != lastCamera.y
            || camera.zoom != lastCamera.zoom;

        wells.resize(planets.size());

        for (std::size_t i = 0; i < planets.size(); ++i) {
            const auto& body = planets[i];
            auto& well = wells[i];

            changed |= well.x != body.getX() || well.y != body.getY();
            well.x = body.getX();
            well.y = body.getY();

            // Depth depends only on mass and type, not on the sampled point.
            if (well.mass != body.getMass() || well.blackHole != body.isBlackHole()) {
                well.mass = body.getMass();
                well.blackHole = body.isBlackHole();

                const double massFactor = std::log1p(std::max(0.0, well.mass) / 5.972e24) / std::log(2.0);
                well.depth = 15.0 * SCALE * massFactor / (1.0 + massFactor);
                if (well.blackHole) well.depth *= 10.0;

                changed = true;
            }
        }

        // Aiming and selecting do not affect the grid. Reuse its vertices.
        if (!changed) return false;

        for (std::size_t line = 0; line < verticalLines.size(); ++line) {

            for (std::size_t point = 0; point < verticalLines[line].size(); ++point) {
                verticalLines[line][point].position =
                    distortPoints(line * 50.0f, point * 5.0f, camera);
            }
        }

        for (std::size_t line = 0; line < horizontalLines.size(); ++line) {

            for (std::size_t point = 0; point < horizontalLines[line].size(); ++point) {
                horizontalLines[line][point].position =
                    distortPoints(point * 5.0f, line * 50.0f, camera);
            }
        }

        lastCamera = camera;
        ready = true;
        return true;
    }

    const std::vector<std::vector<sf::Vertex>>& getVerticalLines() const {
        return verticalLines;
    }

    const std::vector<std::vector<sf::Vertex>>& getHorizontalLines() const {
        return horizontalLines;
    }
};

