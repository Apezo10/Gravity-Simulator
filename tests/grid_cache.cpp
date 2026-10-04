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
        auto now = std::chrono::steady_clock::time_point{};
        std::vector<Planet> bodies = {
            Planet(0, 0, 20, 0, 1, 1.9885e30),
            Planet(1.5e11, 1e10, 0, 0, 1, 5.972e24)
        };

        require(grid.updateGrid(bodies, camera, now), "First frame was not built");
        require(!grid.updateGrid(bodies, camera, now), "Unchanged grid was rebuilt");

        bodies[0].advancePosition(1800);
        const auto cachedPoint = grid.getVerticalLines()[0][0].position;
        now += std::chrono::milliseconds(16);
        require(!grid.updateGrid(bodies, camera, now), "Motion rebuilt grid before refresh deadline");
        require(grid.getVerticalLines()[0][0].position == cachedPoint, "Throttled grid changed vertices");
        bodies[0].advancePosition(1800);
        now += std::chrono::microseconds(17333);
        require(grid.updateGrid(bodies, camera, now), "Pending movement did not refresh at deadline");
        require(std::hypot(grid.getVerticalLines()[0][0].position.x - originalPoint(0, 0, bodies, camera).x,
            grid.getVerticalLines()[0][0].position.y - originalPoint(0, 0, bodies, camera).y) < 0.001f,
            "Refresh used stale body positions");
        camera.x += 1e10;
        require(grid.updateGrid(bodies, camera, now), "Camera X did not refresh grid");
        camera.y -= 2e10;
        require(grid.updateGrid(bodies, camera, now), "Camera Y did not refresh grid");
        camera.zoom = 2;
        require(grid.updateGrid(bodies, camera, now), "Zoom did not refresh grid");

        bodies[1] = Planet(1.5e11, 1e10, 0, 0, 1, 1e31);
        require(grid.updateGrid(bodies, camera, now), "Mass change did not refresh grid");
        bodies[1] = Planet(1.5e11, 1e10, 0, 0, 1, 1e31, {}, true);
        require(grid.updateGrid(bodies, camera, now), "Black hole change did not refresh grid");

        for (double zoom : {0.01, 1.0, 2.0, 10000.0}) {
            camera.zoom = zoom;
            grid.updateGrid(bodies, camera, now);
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
            require(!grid.updateGrid(bodies, camera, now), "Drag flag unnecessarily refreshed grid");
            bodies.emplace_back(0, 0, 0, 0, 1, 1e12);
            require(grid.updateGrid(bodies, camera, now), "Added body did not refresh grid");
            bodies.clear();
            require(grid.updateGrid(bodies, camera, now), "Removed bodies did not refresh grid");
            require(!grid.updateGrid(bodies, camera, now), "Empty grid was not cached");

            for (const sf::Vector2u size : {sf::Vector2u{1280, 720},
                    sf::Vector2u{603, 901}, sf::Vector2u{1, 1}, sf::Vector2u{800, 600}}) {
                camera.resize(size);
                require(grid.updateGrid(bodies, camera, now), "Resize did not refresh paused grid");
                require(!grid.updateGrid(bodies, camera, now), "Resized grid was not cached");
                const auto center = camera.toScreen(camera.x, camera.y);
                require(center == sf::Vector2f(size.x * 0.5f, size.y * 0.5f),
                    "Camera target is not centered after resize");
                const sf::Vector2f cursor(size.x * 0.25f, size.y * 0.75f);
                const auto before = camera.toWorld(cursor);
                camera.scroll(1, cursor);
                const auto after = camera.toWorld(cursor);
                require(std::hypot(before.x - after.x, before.y - after.y) < 0.01,
                    "Zoom moved the world point under the cursor");
                const auto projected = camera.toScreen(after.x, after.y);
                require(std::hypot(projected.x - cursor.x, projected.y - cursor.y) < 0.001,
                    "Screen/world conversion changed after resize");
                camera.reset();
                require(camera.viewport == size, "Reset discarded window size");
                require(camera.toScreen(SCALE, 0).x - camera.toScreen(0, 0).x == 1.f &&
                    camera.toScreen(0, 0).y - camera.toScreen(0, SCALE).y == 1.f,
                    "Resize changed pixel scale or aspect ratio");
                grid.updateGrid(bodies, camera, now);
                require(grid.getVerticalLines().front().back().position.y >= size.y &&
                    grid.getHorizontalLines().front().back().position.x >= size.x,
                    "Grid does not cover resized window");
                for (const auto& vertex : grid.getLineVertices()) {
                    require(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y),
                        "Resized grid contains invalid vertices");
                }
                camera.resize({0, 0});
                require(camera.viewport == size, "Zero-size resize discarded valid viewport");
            }

            std::cout << "Grid cache invalidation and vertex equivalence passed.\n";
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
