#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
using namespace std;

namespace {
sf::Color toColor(BodyColor color) { return {color.r, color.g, color.b}; }
float displayRadius(const Planet& body) {
    const double mass = body.getMass();
    if (mass > 1e29) return 30.0f;
    if (mass > 1e26) return 16.0f;
    if (mass > 1e24) return 10.0f;
    if (mass > 1e23) return 6.0f;
    return 3.0f;
}
class Grid {
    private:
    vector<vector<sf::Vertex>> VectorOfLinesV;
    vector<vector<sf::Vertex>> VectorOfLinesH;

    public:
    void iterateLinesV() {

        //Nested loop to iterate through each of the verticle grid lines
        for (float j=0.0; j<800.0; j+=50.0) {

            vector<sf::Vertex> lineV;

            for (float i=0.0; i<600.0; i+=5.0) {
                lineV.push_back(sf::Vertex{{j, i}});
            }

            VectorOfLinesV.push_back(lineV);
        }
    }

    void iterateLinesH() {

        //Nested loop to iterate through each of the verticle grid lines
        for (float i=0.0; i<600.0; i+=50.0) {

            vector<sf::Vertex> lineH;

            for (float j=0.0; j<800.0; j+=5.0) {
                lineH.push_back(sf::Vertex{{j, i}});
            }

            VectorOfLinesH.push_back(lineH);
        }
    }

    sf::Vector2f distortPoints(float x, float y, const vector<Planet>& planets, const Camera& camera) {

        // Convert the screen sample to world metres before doing any physics-like math.
        const double worldX = camera.x + (x - 400.0) * SCALE / camera.zoom;
        const double worldY = camera.y - (y - 300.0) * SCALE / camera.zoom;
        double dxTotal = 0.0;
        double dyTotal = 0.0;

        for (const Planet& p : planets) {

            const double dx = p.getX() - worldX;
            const double dy = p.getY() - worldY;
            const double distance = max(hypot(dx, dy), 10.0 * SCALE);

            // The grid well has a fixed physical size. Black holes are deeper only
            // as a visual cue; this does not alter the orbital calculations.
            const double massFactor = log1p(max(0.0, p.getMass()) / 5.972e24) / log(2.0);
            double depth = 15.0 * SCALE * massFactor / (1.0 + massFactor);
            if (p.isBlackHole()) depth *= 10.0;
            const double strength = min(depth / (1.0 + distance / (80.0 * SCALE)),
                                        0.9 * distance);

            dxTotal += strength * dx / distance;
            dyTotal += strength * dy / distance;
        }

        const double displacement = hypot(dxTotal, dyTotal);
        const double limit = 150.0 * SCALE;
        if (displacement > limit) {
            dxTotal *= limit / displacement;
            dyTotal *= limit / displacement;
        }

        // Project the distorted world position back through the camera.
        return camera.toScreen(worldX + dxTotal, worldY + dyTotal);
    }

    void updateGrid(const vector<Planet>& planets, const Camera& camera) {
        // Allocate the grid once, then update each vertex in place.
        if (VectorOfLinesV.empty()) iterateLinesV();
        if (VectorOfLinesH.empty()) iterateLinesH();

        for (size_t line = 0; line < VectorOfLinesV.size(); ++line) {
            for (size_t point = 0; point < VectorOfLinesV[line].size(); ++point) {
                VectorOfLinesV[line][point].position =
                    distortPoints(line * 50.0f, point * 5.0f, planets, camera);
            }
        }
        for (size_t line = 0; line < VectorOfLinesH.size(); ++line) {
            for (size_t point = 0; point < VectorOfLinesH[line].size(); ++point) {
                VectorOfLinesH[line][point].position =
                    distortPoints(point * 5.0f, line * 50.0f, planets, camera);
            }
        }
    }
    //Returns the vector as a vector
    const vector<vector<sf::Vertex>>& getLinesV() const {
        return VectorOfLinesV;
    }

    const vector<vector<sf::Vertex>>& getLinesH() const {
        return VectorOfLinesH;
    }
};

class VelocityDisplay {
    sf::Text text;
    sf::RectangleShape background;
    sf::Clock refresh;
    string contents;
public:
    explicit VelocityDisplay(const sf::Font& font) : text(font, "", 14) {
        contents.reserve(2048);
        text.setPosition({12, 10});

        background.setPosition({6, 6});
        background.setFillColor(sf::Color(12, 16, 24, 220));
    }
    void draw(sf::RenderWindow& window, const vector<Planet>& bodies) {
        if (contents.empty() || refresh.getElapsedTime().asSeconds() >= 0.1f) {
            contents = "Velocity (km/s)\n";
            char row[180];
            for (const Planet& body : bodies) {
                snprintf(row, sizeof(row), "%s: %.2f km/s\n",
                    body.getName().c_str(), hypot(body.getXvel(), body.getYvel()) / 1000.0);
                contents += row;
            }
            text.setString(contents);
            auto bounds = text.getLocalBounds();
            background.setSize({bounds.size.x + 18, bounds.size.y + 18});
            refresh.restart();
        }
        window.draw(background);
        window.draw(text);
    }
};

}

struct Renderer::Impl {
    Grid g;
    VelocityDisplay velocityDisplay;
    sf::CircleShape planet;
    vector<sf::Vertex> trailVertices = vector<sf::Vertex>((OrbitTrail::capacity + 1) * 2);
    array<sf::Vertex, 14> trailCapVertices{};
    explicit Impl(const sf::Font& font) : velocityDisplay(font) {}
    void draw(sf::RenderWindow& window, const vector<Planet>& bodies,
              const vector<OrbitTrail>& trails, const Camera& camera) {
        //THIS DRAWS THE GRID
        g.updateGrid(bodies, camera);

        //Draw all the verticle lines
        for (const auto& line : g.getLinesV()) {
            window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
        }

        //Draw all the horizontal lines
        for (const auto& line : g.getLinesH()) {
            window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
        }

        // Draw each body's fading, full-width orbit ribbon.
        for (size_t i = 0; i < trails.size(); ++i) {
            const OrbitTrail& trail = trails[i];
            const Planet& body = bodies[i];
            sf::Color color = body.isBlackHole() ? sf::Color(180, 120, 255) : toColor(body.getColor());

            // Dim the trail's RGB values as well as its alpha, leaving the body bright.
            color.r = static_cast<unsigned char>(color.r * 0.55f);
            color.g = static_cast<unsigned char>(color.g * 0.55f);
            color.b = static_cast<unsigned char>(color.b * 0.55f);

            size_t oldest = (trail.next + OrbitTrail::capacity - trail.count) % OrbitTrail::capacity;
            float radius = displayRadius(body);

            sf::Vector2f previousPoint;
            for (size_t j = 0; j <= trail.count; ++j) {
                auto point = j == trail.count ? camera.toScreen(body.getX(), body.getY())
                    : camera.toScreen(trail.points[(oldest + j) % OrbitTrail::capacity].x,
                                      trail.points[(oldest + j) % OrbitTrail::capacity].y);
                // Measure direction between center points, not the ribbon's edges.
                auto before = j == 0 ? point : previousPoint;
                auto direction = point - before;
                previousPoint = point;

                float length = hypot(direction.x, direction.y);

                sf::Vector2f normal = length > 0.001f ? sf::Vector2f(-direction.y * radius / length, direction.x * radius / length) : sf::Vector2f(0, radius);

                float opacity = 160.0f * (1.0f - static_cast<float>(trail.count - j) / OrbitTrail::capacity);

                color.a = static_cast<unsigned char>(max(0.0f, opacity));
                trailVertices[2 * j] = sf::Vertex{point + normal, color};
                trailVertices[2 * j + 1] = sf::Vertex{point - normal, color};
            }
            if (trail.count > 0) window.draw(trailVertices.data(), (trail.count + 1) * 2, sf::PrimitiveType::TriangleStrip);

            // Round the oldest end with a half-circle matching the ribbon diameter.
            if (trail.count > 1) {
                auto first = camera.toScreen(trail.points[oldest].x, trail.points[oldest].y);
                auto second = camera.toScreen(trail.points[(oldest + 1) % OrbitTrail::capacity].x,
                                              trail.points[(oldest + 1) % OrbitTrail::capacity].y);
                auto tangent = second - first;
                float length = hypot(tangent.x, tangent.y);

                if (length > 0.001f) {
                    tangent /= length;
                    color.a = 30; // The oldest end is faint, like the ribbon behind it.

                    trailCapVertices[0] = sf::Vertex{first, color};
                    constexpr int segments = 12;

                    for (int segment = 0; segment <= segments; ++segment) {
                        float angle = atan2(tangent.y, tangent.x) + 0.5f * 3.14159265f
                                    + 3.14159265f * segment / segments;
                        sf::Vector2f edge(first.x + radius * cos(angle),
                                          first.y + radius * sin(angle));
                        trailCapVertices[segment + 1] = sf::Vertex{edge, color};
                    }
                    window.draw(trailCapVertices.data(), segments + 2, sf::PrimitiveType::TriangleFan);
                }
            }
        }

        //______________________________________________________________________________
        // GO THROUGH EVERY PLANET IN THE SYSTEM AND DRAW
        for (const Planet& body : bodies) {

            // Fixed pixel radii keep small bodies visible without changing physics.
            float r = displayRadius(body);
            auto position = camera.toScreen(body.getX(), body.getY());

            planet.setRadius(r);
            planet.setOrigin({r,r});
            planet.setFillColor(toColor(body.getColor()));
            // Outline makes the black marker visible against the black background.
            planet.setOutlineThickness(body.isBlackHole() ? 2.0f : 0.0f);
            planet.setOutlineColor(sf::Color(180, 120, 255));

            planet.setPosition(position);

            window.draw(planet);
        }

        velocityDisplay.draw(window, bodies);

    }
};
Renderer::Renderer(const sf::Font& font) : impl(std::make_unique<Impl>(font)) {}
Renderer::~Renderer() = default;
void Renderer::draw(sf::RenderWindow& window, const vector<Planet>& bodies,
                    const vector<OrbitTrail>& trails, const Camera& camera) {
    impl->draw(window, bodies, trails, camera);
}
