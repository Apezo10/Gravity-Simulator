#include "renderer.hpp"
#include "grid.hpp"
#include "trajectory_preview.hpp"
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
                session.launch ? "Aiming" : (session.paused ? "Paused" : "Running"), session.speed(),
                session.elapsedSeconds / 86400.0, session.system.getBodies().size());
            contents = row;
            contents += "Space: pause | Up/Down: speed | R: reset\n"
            "Shift-click: select | F: follow | Esc: deselect\n"
            "Left-drag: launch asteroid | Right-drag: pan | Wheel: zoom\n";

            if (session.paused || session.launch) {
                contents += "Dotted forecast: up to 90 days / first collision\n";
            }

            if (session.launch) {
                snprintf(row, sizeof(row), "Launch: %.1f km/s | Release: launch | Esc: cancel\n",
                    hypot(session.launch->vx, session.launch->vy) / 1000.0);
                contents += row;
            }


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
    TrajectoryPreview preview;
    vector<Planet> previewBodies;
    bool previewReady = false;
    std::uint64_t previewRevision = 0;
    sf::Clock previewRefresh;
    bool previewWasLaunching = false;

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

    void drawPreview(sf::RenderWindow& window, const SimulationSession& session,
        const Camera& camera) {
        if (!session.paused && !session.launch) {
            previewReady = false;
            return;
        }

        const bool launching = session.launch.has_value();
        const bool modeChanged = launching != previewWasLaunching;
        const bool stateChanged = previewRevision != session.stateRevision();

        // Recalculate at most ten times a second while aiming. The arrow still
        // follows every frame, and the latest aim is used when the timer expires.
        const bool refreshDue = !launching || previewRefresh.getElapsedTime().asMilliseconds() >= 100;

        if (!previewReady || modeChanged || (stateChanged && refreshDue)) {
            PlanetSystem forecast = session.system;

            if (session.launch) {
                const auto& launch = *session.launch;
                forecast.addAsteroid(launch.x, launch.y, launch.vx, launch.vy);
            }

            previewBodies = forecast.getBodies();
            preview.calculate(forecast);
            previewRevision = session.stateRevision();
            previewReady = true;
            previewWasLaunching = launching;
            previewRefresh.restart();
        }

        const auto& bodies = previewBodies;
        sf::CircleShape dot(2.0f, 12);
        dot.setOrigin({2.0f, 2.0f});

        for (size_t i = 0; i < bodies.size(); ++i) {
            if (session.launch ? i + 1 != bodies.size()
                               : (session.selected && *session.selected != i)) {
                continue;
            }

            const auto& path = preview.paths[i];
            auto color = bodies[i].isBlackHole()
                ? sf::Color(180, 120, 255) : toColor(bodies[i].getColor());
            auto lastDot = camera.toScreen(bodies[i].getX(), bodies[i].getY());

            for (size_t j = 0; j < path.size(); ++j) {
                const auto point = camera.toScreen(path[j].x, path[j].y);

                // Keep nearby samples from becoming a solid line when zoomed out.
                if (hypot(point.x - lastDot.x, point.y - lastDot.y) < 7.0f) {
                    continue;
                }

                color.a = static_cast<unsigned char>(210 - 150 * j / path.size());
                dot.setFillColor(color);
                dot.setPosition(point);
                window.draw(dot);
                lastDot = point;
            }
        }
    }

    void drawLaunch(sf::RenderWindow& window, const SimulationSession& session,
        const Camera& camera) {
        if (!session.launch) return;

        const auto& launch = *session.launch;
        const auto start = camera.toScreen(launch.x, launch.y);
        const sf::Vector2f direction(
            static_cast<float>(launch.vx / AsteroidLaunch::speedPerPixel),
            static_cast<float>(-launch.vy / AsteroidLaunch::speedPerPixel));
        const auto end = start + direction;
        const sf::Color color(255, 190, 100);

        sf::CircleShape marker(5.0f, 24);
        marker.setOrigin({5.0f, 5.0f});
        marker.setPosition(start);
        marker.setFillColor(sf::Color(255, 165, 0, 100));
        marker.setOutlineColor(color);
        marker.setOutlineThickness(1.5f);
        window.draw(marker);

        const float length = hypot(direction.x, direction.y);
        if (length < 3.0f) return;

        const auto unit = direction / length;
        const sf::Vector2f normal(-unit.y, unit.x);
        const float headSize = min(10.0f, length * 0.4f);

        const array<sf::Vertex, 6> arrow = {{
            {start, color}, {end, color},
            {end, color}, {end - unit * headSize + normal * headSize * 0.5f, color},
            {end, color}, {end - unit * headSize - normal * headSize * 0.5f, color}
        }};
        window.draw(arrow.data(), arrow.size(), sf::PrimitiveType::Lines);
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
        drawPreview(window, session, camera);
        drawBodies(window, bodies, camera);
        drawSelection(window, session, camera);
        drawLaunch(window, session, camera);
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
