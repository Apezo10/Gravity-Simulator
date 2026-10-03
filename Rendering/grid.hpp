#pragma once

#include "camera.hpp"
#include "planet.hpp"
#include <SFML/Graphics/Vertex.hpp>
#include <limits>
#include <vector>

class Grid {

private:
    std::vector<std::vector<sf::Vertex>> verticalLines;
    std::vector<std::vector<sf::Vertex>> horizontalLines;
    std::vector<sf::Vertex> lineVertices;

    static double length(double x, double y) {
        const double squared = x * x + y * y;
        // Use the cheaper square root at ordinary simulation distances, while
        // retaining hypot's overflow/underflow protection for extreme inputs.
        if (std::isfinite(squared) && squared >= std::numeric_limits<double>::min()) {
            return std::sqrt(squared);
        }
        return std::hypot(x, y);
    }

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
            const double distance = std::max(length(dx, dy), 10.0 * SCALE);

            const double strength = std::min(well.depth / (1.0 + distance / (80.0 * SCALE)),
                0.9 * distance);

            const double weight = strength / distance;
            dxTotal += weight * dx;
            dyTotal += weight * dy;
        }

        const double displacement = length(dxTotal, dyTotal);
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
          horizontalLines(12, std::vector<sf::Vertex>(160)),
          lineVertices(2 * (16 * 119 + 12 * 159)) {}

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
                    point % 10 == 0 ? verticalLines[point / 10][line * 10].position
                                   : distortPoints(point * 5.0f, line * 50.0f, camera);
            }
        }

        // Independent segments let every grid line share one draw call without
        // connecting the end of one line to the beginning of the next.
        std::size_t vertex = 0;
        for (const auto* lines : {&verticalLines, &horizontalLines}) {
            for (const auto& line : *lines) {
                for (std::size_t point = 1; point < line.size(); ++point) {
                    lineVertices[vertex++] = line[point - 1];
                    lineVertices[vertex++] = line[point];
                }
            }
        }

        lastCamera = camera;
        ready = true;
        return true;
    }

    const std::vector<std::vector<sf::Vertex>>& getVerticalLines() const {
        return verticalLines;
    }

    const std::vector<sf::Vertex>& getLineVertices() const {
        return lineVertices;
    }

    const std::vector<std::vector<sf::Vertex>>& getHorizontalLines() const {
        return horizontalLines;
    }
};

