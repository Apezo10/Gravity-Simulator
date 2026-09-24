#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace std;

namespace {
    sf::Color toColor(BodyColor color) {
        return {
            color.r, color.g, color.b};
    }

    float displayRadius(const Planet& body) {
        const double mass = body.getMass();


        if (mass > 1e29) {
            return 30.0f;
        }


        if (mass > 1e26) {
            return 16.0f;
        }


        if (mass > 1e24) {
            return 10.0f;
        }


        if (mass > 1e23) {
            return 6.0f;
        }

        return 3.0f;
    }

    class Grid {

    private:
        vector<vector<sf::Vertex>> verticalLines;
        vector<vector<sf::Vertex>> horizontalLines;

    public:
        void createVerticalLines() {


            for (float j=0.0; j<800.0; j+=50.0) {

                vector<sf::Vertex> lineV;


                for (float i=0.0; i<600.0; i+=5.0) {
                    lineV.push_back(sf::Vertex{
                            {
                                j, i}});
                }

                verticalLines.push_back(lineV);
            }
        }

        void createHorizontalLines() {


            for (float i=0.0; i<600.0; i+=50.0) {

                vector<sf::Vertex> lineH;


                for (float j=0.0; j<800.0; j+=5.0) {
                    lineH.push_back(sf::Vertex{
                            {
                                j, i}});
                }

                horizontalLines.push_back(lineH);
            }
        }

        sf::Vector2f distortPoints(float x, float y, const vector<Planet>& planets, const Camera& camera) {

            // Convert the screen sample to world metres before doing any physics-like math.
            const auto world = camera.toWorld({
                    x, y});
            const double worldX = world.x;
            const double worldY = world.y;
            double dxTotal = 0.0;
            double dyTotal = 0.0;


            for (const Planet& body : planets) {

                const double dx = body.getX() - worldX;
                const double dy = body.getY() - worldY;
                const double distance = max(hypot(dx, dy), 10.0 * SCALE);

                // The grid well has a fixed physical size. Black holes are deeper only
                // as a visual cue; this does not alter the orbital calculations.
                const double massFactor = log1p(max(0.0, body.getMass()) / 5.972e24) / log(2.0);
                double depth = 15.0 * SCALE * massFactor / (1.0 + massFactor);


                if (body.isBlackHole()) {
                    depth *= 10.0;
                }

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
            if (verticalLines.empty()) {
                createVerticalLines();
            }


            if (horizontalLines.empty()) {
                createHorizontalLines();
            }


            for (size_t line = 0; line < verticalLines.size(); ++line) {


                for (size_t point = 0; point < verticalLines[line].size(); ++point) {
                    verticalLines[line][point].position =
                    distortPoints(line * 50.0f, point * 5.0f, planets, camera);
                }
            }


            for (size_t line = 0; line < horizontalLines.size(); ++line) {


                for (size_t point = 0; point < horizontalLines[line].size(); ++point) {
                    horizontalLines[line][point].position =
                    distortPoints(point * 5.0f, line * 50.0f, planets, camera);
                }
            }
        }

        const vector<vector<sf::Vertex>>& getVerticalLines() const {
            return verticalLines;
        }

        const vector<vector<sf::Vertex>>& getHorizontalLines() const {
            return horizontalLines;
        }
    };

    class StatusDisplay {
        sf::Text text;
        sf::RectangleShape background;
        string contents;

    public:
        explicit StatusDisplay(const sf::Font& font) : text(font, "", 14) {
            contents.reserve(2048);
            text.setPosition({
                    12, 10});

            background.setPosition({
                    6, 6});
            background.setFillColor(sf::Color(12, 16, 24, 220));
        }

        void draw(sf::RenderWindow& window, const SimulationSession& session) {
            char row[320];
            snprintf(row, sizeof(row), "%s | %.2gx | Day %.2f | %zu bodies\n",
                session.paused ? "Paused" : "Running", session.speed(),
                session.elapsedSeconds / 86400.0, session.system.getBodies().size());
            contents = row;
            contents += "Space: pause | Up/Down: speed | R: reset\n"
            "Shift-click: select | F: follow | Esc: deselect\n"
            "Left-click: asteroid | Right-drag: pan | Wheel: zoom\n";


            if (session.selected && *session.selected < session.system.getBodies().size()) {
                const auto& body = session.system.getBodies()[*session.selected];
                contents += "\n" + body.getName() + (session.following ? " (following)\n" : "\n");
                snprintf(row, sizeof(row), "Mass: %.3e kg | Speed: %.2f km/s\nPosition: (%.3e, %.3e) m",
                    body.getMass(), hypot(body.getXVelocity(), body.getYVelocity()) / 1000.0,
                    body.getX(), body.getY());
                contents += row;
            } else {
                contents += "\nShift-click a body to inspect it.";
            }

            text.setString(contents);
            auto bounds = text.getLocalBounds();
            background.setSize({
                    bounds.size.x + 18, bounds.size.y + 18});
            window.draw(background);
            window.draw(text);
        }
    };

}

struct Renderer::Impl {
    Grid grid;
    StatusDisplay statusDisplay;
    sf::CircleShape planet;
    vector<sf::Vertex> trailVertices = vector<sf::Vertex>((OrbitTrail::capacity + 1) * 2);
    array<sf::Vertex, 14> trailCapVertices{
    };
    explicit Impl(const sf::Font& font) : statusDisplay(font) {}
    void drawGrid(sf::RenderWindow& window, const vector<Planet>& bodies, const Camera& camera) {

        // Update the decorative grid before drawing it.
        grid.updateGrid(bodies, camera);


        // Draw the vertical lines.
        for (const auto& line : grid.getVerticalLines()) {
            window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
        }


        // Draw the horizontal lines.
        for (const auto& line : grid.getHorizontalLines()) {
            window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
        }


    }

    void drawTrails(sf::RenderWindow& window, const vector<Planet>& bodies,
        const vector<OrbitTrail>& trails, const Camera& camera) {


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

                sf::Vector2f normal(0, radius);


                if (length > 0.001f) {
                    normal = {
                        -direction.y * radius / length, direction.x * radius / length};
                }

                float opacity = 160.0f * (1.0f - static_cast<float>(trail.count - j) / OrbitTrail::capacity);

                color.a = static_cast<unsigned char>(max(0.0f, opacity));
                trailVertices[2 * j] = sf::Vertex{
                    point + normal, color};
                trailVertices[2 * j + 1] = sf::Vertex{
                    point - normal, color};
            }


            if (trail.count > 0) {
                window.draw(trailVertices.data(), (trail.count + 1) * 2, sf::PrimitiveType::TriangleStrip);
            }


            // Round the oldest end with a half-circle matching the ribbon diameter.
            if (trail.count > 1) {
                auto first = camera.toScreen(trail.points[oldest].x, trail.points[oldest].y);
                auto second = camera.toScreen(trail.points[(oldest + 1) % OrbitTrail::capacity].x,
                    trail.points[(oldest + 1) % OrbitTrail::capacity].y);
                auto tangent = second - first;
                float length = hypot(tangent.x, tangent.y);


                if (length > 0.001f) {
                    tangent /= length;

                    // The oldest end is faint, like the ribbon behind it.
                    color.a = 30;

                    trailCapVertices[0] = sf::Vertex{
                        first, color};
                    constexpr int segments = 12;


                    for (int segment = 0; segment <= segments; ++segment) {
                        float angle = atan2(tangent.y, tangent.x) + 0.5f * 3.14159265f
                        + 3.14159265f * segment / segments;
                        sf::Vector2f edge(first.x + radius * cos(angle),
                            first.y + radius * sin(angle));
                        trailCapVertices[segment + 1] = sf::Vertex{
                            edge, color};
                    }

                    window.draw(trailCapVertices.data(), segments + 2, sf::PrimitiveType::TriangleFan);
                }
            }
        }

    }

    void drawBodies(sf::RenderWindow& window, const vector<Planet>& bodies, const Camera& camera) {


        for (const Planet& body : bodies) {

            // Fixed pixel radii keep small bodies visible without changing physics.
            float r = displayRadius(body);
            auto position = camera.toScreen(body.getX(), body.getY());

            planet.setRadius(r);
            planet.setOrigin({
                    r,r});
            planet.setFillColor(toColor(body.getColor()));

            // Outline makes the black marker visible against the black background.
            planet.setOutlineThickness(body.isBlackHole() ? 2.0f : 0.0f);
            planet.setOutlineColor(sf::Color(180, 120, 255));

            planet.setPosition(position);

            window.draw(planet);
        }


    }

    void drawSelection(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera) {
        const auto& bodies = session.system.getBodies();


        if (session.selected && *session.selected < bodies.size()) {
            const auto& body = bodies[*session.selected];
            const float radius = displayRadius(body) + 5;
            planet.setRadius(radius);
            planet.setOrigin({
                    radius, radius});
            planet.setPosition(camera.toScreen(body.getX(), body.getY()));
            planet.setFillColor(sf::Color::Transparent);
            planet.setOutlineColor(sf::Color::White);
            planet.setOutlineThickness(1.5f);
            window.draw(planet);
        }

    }

    void draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera) {
        const auto& bodies = session.system.getBodies();
        drawGrid(window, bodies, camera);
        drawTrails(window, bodies, session.trails, camera);
        drawBodies(window, bodies, camera);
        drawSelection(window, session, camera);
        statusDisplay.draw(window, session);
    }
};
Renderer::Renderer(const sf::Font& font) : impl(std::make_unique<Impl>(font)) {}
Renderer::~Renderer() = default;
void Renderer::draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera) {
    impl->draw(window, session, camera);
}

optional<size_t> Renderer::pickBody(sf::Vector2f mouse, const vector<Planet>& bodies,
    const Camera& camera) const {


    // Select the topmost visible marker when markers overlap.
    for (size_t i = bodies.size(); i-- > 0;) {
        const auto point = camera.toScreen(bodies[i].getX(), bodies[i].getY());


        if (hypot(mouse.x - point.x, mouse.y - point.y) <= displayRadius(bodies[i]) + 5)
        {
            return i;
        }
    }

    return nullopt;
}
