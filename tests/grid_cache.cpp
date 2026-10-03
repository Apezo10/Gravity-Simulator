#include "grid.hpp"
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// Original per-point calculation checks that caching preserves the appearance.
sf::Vector2f originalPoint(float x, float y, const std::vector<Planet>& bodies,
    const Camera& camera) {
    const auto world = camera.toWorld({x, y});
    double dxTotal = 0, dyTotal = 0;

    for (const auto& body : bodies) {
        const double dx = body.getX() - world.x;
        const double dy = body.getY() - world.y;
        const double distance = std::max(std::hypot(dx, dy), 10.0 * SCALE);
        const double factor = std::log1p(std::max(0.0, body.getMass()) / 5.972e24) / std::log(2.0);
        double depth = 15.0 * SCALE * factor / (1.0 + factor);
        if (body.isBlackHole()) depth *= 10.0;

        const double strength = std::min(depth / (1.0 + distance / (80.0 * SCALE)), 0.9 * distance);
        dxTotal += strength * dx / distance;
        dyTotal += strength * dy / distance;
    }

    const double displacement = std::hypot(dxTotal, dyTotal);
    const double limit = 150.0 * SCALE;
    if (displacement > limit) {
        dxTotal *= limit / displacement;
        dyTotal *= limit / displacement;
    }

    return camera.toScreen(world.x + dxTotal, world.y + dyTotal);
}

int main() {
    try {
        Grid grid;
        Camera camera;
        std::vector<Planet> bodies = {
            Planet(0, 0, 20, 0, 1, 1.9885e30),
            Planet(1.5e11, 1e10, 0, 0, 1, 5.972e24)
        };

        require(grid.updateGrid(bodies, camera), "First frame was not built");
        require(!grid.updateGrid(bodies, camera), "Unchanged grid was rebuilt");

        bodies[0].advancePosition(1800);
        require(grid.updateGrid(bodies, camera), "Body movement did not refresh grid");
        camera.x += 1e10;
        require(grid.updateGrid(bodies, camera), "Camera X did not refresh grid");
        camera.y -= 2e10;
        require(grid.updateGrid(bodies, camera), "Camera Y did not refresh grid");
        camera.zoom = 2;
        require(grid.updateGrid(bodies, camera), "Zoom did not refresh grid");

        bodies[1] = Planet(1.5e11, 1e10, 0, 0, 1, 1e31);
        require(grid.updateGrid(bodies, camera), "Mass change did not refresh grid");
        bodies[1] = Planet(1.5e11, 1e10, 0, 0, 1, 1e31, {}, true);
        require(grid.updateGrid(bodies, camera), "Black hole change did not refresh grid");

        for (double zoom : {0.01, 1.0, 2.0, 10000.0}) {
            camera.zoom = zoom;
            grid.updateGrid(bodies, camera);
            std::size_t segmentVertex = 0;
            for (int axis = 0; axis < 2; ++axis) {
                const auto& lines = axis == 0 ? grid.getVerticalLines() : grid.getHorizontalLines();

                for (std::size_t line = 0; line < lines.size(); ++line) {
                    for (std::size_t point = 0; point < lines[line].size(); ++point) {
                        const float x = axis == 0 ? line * 50.0f : point * 5.0f;
                        const float y = axis == 0 ? point * 5.0f : line * 50.0f;
                        const auto expected = originalPoint(x, y, bodies, camera);
                        const auto actual = lines[line][point].position;
                        require(std::hypot(actual.x - expected.x, actual.y - expected.y) < 0.001f,
                            "Cached grid changed the original appearance");
                        if (point > 0) {
                            const auto& vertices = grid.getLineVertices();
                            require(segmentVertex + 1 < vertices.size(), "Missing grid segment");
                            require(vertices[segmentVertex++].position == lines[line][point - 1].position,
                                "Grid segment begins on the wrong line");
                            require(vertices[segmentVertex++].position == actual,
                                "Grid segment ends at the wrong point");
                        }
                    }
                }
            }
            require(segmentVertex == grid.getLineVertices().size(), "Extra grid segments");
            }

            camera.dragging = true;
            require(!grid.updateGrid(bodies, camera), "Drag flag unnecessarily refreshed grid");
            bodies.emplace_back(0, 0, 0, 0, 1, 1e12);
            require(grid.updateGrid(bodies, camera), "Added body did not refresh grid");
            bodies.clear();
            require(grid.updateGrid(bodies, camera), "Removed bodies did not refresh grid");
            require(!grid.updateGrid(bodies, camera), "Empty grid was not cached");

            std::cout << "Grid cache invalidation and vertex equivalence passed.\n";
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
