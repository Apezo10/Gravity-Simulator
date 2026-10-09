#include "renderer.hpp"
#include "grid.hpp"
#include "trajectory_preview.hpp"
#include "performance_stats.hpp"
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
        sf::Clock accuracyRefresh;
        AccuracyStats accuracy;
        AccuracyStats displayedBaseline;
        bool accuracyVisible = false;
        double displayedCollisionLoss = 0;

    public:
        explicit StatusDisplay(const sf::Font& font) : text(font, "", 14) {
            contents.reserve(2048);
            text.setPosition({
                    12, 10});

            background.setPosition({
                    6, 6});
            background.setFillColor(sf::Color(12, 16, 24, 220));
        }

        void draw(sf::RenderWindow& window, const SimulationSession& session,
            const PerformanceStats& performance) {
            char row[320];
            snprintf(row, sizeof(row), "%s | %.2gx | Day %.2f | %zu bodies\n",
                session.launch ? "Aiming" : (session.paused ? "Paused" : "Running"), session.speed(),
                session.elapsedSeconds / 86400.0, session.system.getBodies().size());
            contents = row;
            contents += "Space: pause | Up/Down: speed | R: reset\n"
            "Shift-click: select | F: follow | Esc: deselect\n"
            "Left-drag: launch asteroid | Right-drag: pan | Wheel: zoom\n"
            "F3: performance | F4: accuracy | Home: fit all bodies\n"
            "Q/E: mass /10 or x10 | Z/X: radius /2 or x2\n";
            snprintf(row, sizeof(row), "Next launch: %.3e kg | Radius: %.3e m\n",
                session.asteroidMass(), session.asteroidRadius());
            contents += row;

            if (session.showAccuracy) {
                if (!accuracyVisible || accuracyRefresh.getElapsedTime().asMilliseconds() >= 500) {
                    accuracy = measureAccuracy(session.system.getBodies());
                    displayedBaseline = session.getAccuracyBaseline();
                    displayedCollisionLoss = session.system.getCollisionKineticLoss();
                    accuracyRefresh.restart();
                }
                const auto& baseline = displayedBaseline;
                contents += "Accuracy: since last reset, launch or merge\n";
                if (accuracy.energyDefined && baseline.energyDefined) {
                    const double drift = accuracy.energy - baseline.energy;
                    snprintf(row, sizeof(row), "Energy: %.3e J | Drift: %+.3e J\n", accuracy.energy, drift);
                    contents += row;
                    if (baseline.energyScale > 0) {
                        snprintf(row, sizeof(row), "Relative drift: %+.3e (K + |U| scale)\n", drift / baseline.energyScale);
                        contents += row;
                    }
                } else contents += "Energy: undefined for coincident/extreme bodies\n";
                const auto momentumDrift = measureMomentumDrift(accuracy, baseline);
                if (accuracy.momentumDefined) {
                    snprintf(row, sizeof(row), "Momentum: (%.3e, %.3e) kg m/s\n",
                        accuracy.momentumX, accuracy.momentumY);
                    contents += row;
                } else contents += "Momentum: undefined for extreme bodies\n";
                if (momentumDrift.defined) {
                    snprintf(row, sizeof(row), "Momentum drift: %.3e kg m/s\n", momentumDrift.magnitude);
                    contents += row;
                    if (momentumDrift.relativeDefined) {
                        snprintf(row, sizeof(row), "Relative momentum drift: %.3e (sum |p| scale)\n",
                            momentumDrift.relative);
                        contents += row;
                    }
                } else contents += "Momentum drift: undefined for extreme bodies\n";
                snprintf(row, sizeof(row), "Merge kinetic loss: %.3e J\n", displayedCollisionLoss);
                contents += row;
            }
            accuracyVisible = session.showAccuracy;

            if (session.showPerformance) {
                const auto& p = performance.average;
                snprintf(row, sizeof(row), "%.1f FPS | Frame: %.2f ms (0.5 s average)\n"
                    "CPU ms: Physics %.3f | Render submission %.3f\n"
                    "Render breakdown: Grid %.3f | Trails %.3f | Preview %.3f\n",
                    performance.fps, p.frameMs, p.physicsMs, p.renderMs, p.gridMs, p.trailsMs, p.previewMs);
                contents += row;
            }

            if (session.paused || session.launch) {
                contents += "Dotted forecast: up to 90 days, including merges\n";
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
    PerformanceStats performance;
    sf::CircleShape planet;
    TrajectoryPreview preview;
    vector<Planet> previewBodies;
    vector<vector<PredictedPoint>> retainedPreviewPaths;
    bool previewReady = false;
    std::uint64_t previewRevision = 0;
    sf::Clock previewRefresh;
    bool previewWasLaunching = false;

    vector<sf::Vertex> previewVertices;
    const array<sf::Vector2f, 12> dotOutline = [] {
        // Match the original two-pixel circle, calculating its outline only once.
        const sf::CircleShape dot(2.0f, 12);
        array<sf::Vector2f, 12> points;

        for (size_t i = 0; i < points.size(); ++i) {
            points[i] = dot.getPoint(i) - sf::Vector2f(2.0f, 2.0f);
        }

        return points;
    }();

    vector<sf::Vertex> trailVertices = vector<sf::Vertex>((OrbitTrail::capacity + 1) * 2);
    array<sf::Vertex, 14> trailCapVertices{
    };
    explicit Impl(const sf::Font& font) : statusDisplay(font) {}
    void drawGrid(sf::RenderWindow& window, const vector<Planet>& bodies, const Camera& camera) {

        // Update the decorative grid before drawing it.
        grid.updateGrid(bodies, camera);


        const auto& vertices = grid.getLineVertices();
        window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);


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
            retainedPreviewPaths.clear();
            return;
        }

        const bool launching = session.launch.has_value();
        const bool modeChanged = launching != previewWasLaunching;
        const bool stateChanged = previewRevision != session.stateRevision();

        // Replace outdated forecasts at most ten times a second while aiming.
        // Keep the previous aiming path until replacement samples are ready.
        const bool propertiesChanged = launching && !previewBodies.empty() &&
            (previewBodies.back().getMass() != session.launch->mass ||
             previewBodies.back().getRadius() != session.launch->radius);
        const bool refreshDue = propertiesChanged || !launching ||
            previewRefresh.getElapsedTime().asMilliseconds() >= 100;

        if (!previewReady || modeChanged || (stateChanged && refreshDue)) {
            PlanetSystem forecast = session.system;

            if (session.launch) {
                const auto& launch = *session.launch;
                launch.addTo(forecast);
            }

            if (previewReady && launching && !modeChanged && !propertiesChanged &&
                forecast.getBodies().size() == previewBodies.size()) {
                if (preview.hasSamples()) retainedPreviewPaths = preview.paths;
            } else {
                retainedPreviewPaths.clear();
            }
            previewBodies = forecast.getBodies();
            preview.begin(forecast);
            previewRevision = session.stateRevision();
            previewReady = true;
            previewWasLaunching = launching;
            previewRefresh.restart();
        }

        preview.advance();
        if (preview.hasSamples()) retainedPreviewPaths.clear();

        const auto& bodies = previewBodies;
        const auto& visiblePaths = retainedPreviewPaths.empty() ? preview.paths : retainedPreviewPaths;

        // Keep the allocated storage between frames and batch all visible dots.
        previewVertices.clear();

        for (size_t i = 0; i < bodies.size(); ++i) {
            if (session.launch ? i + 1 != bodies.size()
                               : (session.selected && *session.selected != i)) {
                continue;
            }

            const auto& path = visiblePaths[i];
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

                for (size_t edge = 0; edge < dotOutline.size(); ++edge) {
                    previewVertices.push_back({point, color});
                    previewVertices.push_back({point + dotOutline[edge], color});
                    previewVertices.push_back({point + dotOutline[(edge + 1) % dotOutline.size()], color});
                }

                lastDot = point;
            }
        }

        if (!previewVertices.empty()) {
            window.draw(previewVertices.data(), previewVertices.size(), sf::PrimitiveType::Triangles);
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

        const double radiusPixels = launch.radius * camera.zoom / SCALE;
        // Avoid building a huge off-screen outline when zoomed far into a body.
        if (radiusPixels >= 6 && radiusPixels <= 2.0 * std::max(camera.viewport.x, camera.viewport.y)) {
            const float radius = static_cast<float>(radiusPixels);
            sf::CircleShape footprint(radius, 96);
            footprint.setOrigin({radius, radius});
            footprint.setPosition(start);
            footprint.setFillColor(sf::Color::Transparent);
            footprint.setOutlineColor(sf::Color(255, 190, 100, 110));
            footprint.setOutlineThickness(1.f);
            window.draw(footprint);
        }

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

    void draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera,
        double frameMs, double physicsMs) {
        const auto renderStart = std::chrono::steady_clock::now();
        const auto& bodies = session.system.getBodies();
        PerformanceSample sample;
        sample.frameMs = frameMs;
        sample.physicsMs = physicsMs;
        sample.gridMs = measureMilliseconds([&] { drawGrid(window, bodies, camera); });
        sample.trailsMs = measureMilliseconds([&] { drawTrails(window, bodies, session.trails, camera); });
        sample.previewMs = measureMilliseconds([&] { drawPreview(window, session, camera); });
        drawBodies(window, bodies, camera);
        drawSelection(window, session, camera);
        drawLaunch(window, session, camera);
        statusDisplay.draw(window, session, performance);
        // Include markers and HUD (including accuracy calculations), which
        // are absent from the component timers. Presentation and frame-cap
        // waiting happen in main, outside this render-submission measurement.
        sample.renderMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - renderStart).count();
        performance.add(sample);
    }
};
Renderer::Renderer(const sf::Font& font) : impl(std::make_unique<Impl>(font)) {}
Renderer::~Renderer() = default;
void Renderer::draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera,
    double frameMs, double physicsMs) {
    impl->draw(window, session, camera, frameMs, physicsMs);
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
