#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>
#include <random>
#include <array>
#include <sstream>
#include <cstdint>
#include <utility>
#include <SFML/Graphics.hpp>
using namespace std;

// Use the local font when compiling without CMake's font path definition.
#ifndef SIM_FONT_PATH
#define SIM_FONT_PATH "assets/tuffy.ttf"
#endif

//______________________________________________________________________________
// CONSTANTS

//Universal constant
const double G = 6.67430e-11;

//Scale metres to pixals
const double SCALE = 2.0e9;

// Match the former speed at 60 FPS: 30 simulated hours per real second.
constexpr double PHYSICS_STEP_SECONDS = 1800.0;
class SimulationTiming {
    std::int64_t pendingTicks = 0;
public:
    template <typename Step>
    void advance(std::int64_t elapsedMicroseconds, Step step) {
        // Limit catch-up after a long stall to 15 steps (a quarter second).
        // Discard excess wall time instead of freezing the UI in a backlog.
        pendingTicks += clamp<std::int64_t>(elapsedMicroseconds, 0, 250000) * 60;
        while (pendingTicks >= 1000000) {
            step(PHYSICS_STEP_SECONDS);
            pendingTicks -= 1000000;
        }
    }
};

// Read a complete line so malformed values cannot spill into the next field.
// A closed input stream cancels setup instead of retrying forever.
template <typename T, typename Validator>
bool readNumber(const char* prompt, T& value, Validator valid, const char* error) {
    string line;
    while (true) {
        cout << prompt;
        if (!getline(cin, line)) return false;
        istringstream input(line);
        T candidate{};
        if (input >> candidate) {
            input >> ws;
            if (input.eof() && valid(candidate)) {
                value = candidate;
                return true;
            }
        }
        cout << error << '\n';
    }
}

//______________________________________________________________________________
// CAMERA: PANNING, ZOOMING AND SCREEN POSITIONS
// Camera position is in metres; screen coordinates stay in pixels.
struct Camera {
    double x = 0, y = 0, zoom = 1;
    bool dragging = false;
    sf::Vector2f lastMouse;

    sf::Vector2f toScreen(double px, double py) const {
        return {400.0f + static_cast<float>((px - x) * zoom / SCALE),
                300.0f - static_cast<float>((py - y) * zoom / SCALE)};
    }

    void drag(sf::Vector2f mouse) {
        x -= (mouse.x - lastMouse.x) * SCALE / zoom;
        y += (mouse.y - lastMouse.y) * SCALE / zoom;
        lastMouse = mouse;
    }

    void scroll(float delta, sf::Vector2f mouse) {
        double oldScale = SCALE / zoom;
        zoom = clamp(zoom * pow(1.25, delta), 0.01, 10000.0);
        double newScale = SCALE / zoom;
        // Keep the world point under the cursor stationary while zooming.
        x += (mouse.x - 400) * (oldScale - newScale);
        y -= (mouse.y - 300) * (oldScale - newScale);
    }
};

//______________________________________________________________________________
// PLANET: POSITION, VELOCITY AND APPEARANCE
//Simple class which describes the state of a single particle
class Planet {
    private:
    double xPos{};
    double yPos{};
    double xVel{};
    double yVel{};
    double radius{};
    double xAccel{0};
    double yAccel{0};
    double mass{};
    sf::Color color{135, 206, 235};
    bool blackHole = false;
    string name;

    public:
    Planet() = default;

    Planet(double x, double y, double vx, double vy, double r, double m,
           sf::Color tint = sf::Color(135, 206, 235), bool isBlackHole = false,
           string bodyName = "")
        : xPos(x), yPos(y), xVel(vx), yVel(vy), radius(r), mass(m),
          color(tint), blackHole(isBlackHole), name(bodyName) {}

    sf::Color getColor() const { return color; }
    bool isBlackHole() const { return blackHole; }
    string getName() const {
        if (!name.empty()) return name;
        if (blackHole) return "Black hole";
        if (mass >= 1.59e29) return "Star";
        if (mass >= 1e26) return "Giant planet";
        if (mass >= 1e24) return "Terrestrial planet";
        if (mass >= 1e23) return "Small planet";
        return "Moon or small body";
    }
    float getDisplayRadius() const {
        if (mass > 1e29) return 30.0f;
        if (mass > 1e26) return 16.0f;
        if (mass > 1e24) return 10.0f;
        if (mass > 1e23) return 6.0f;
        return 3.0f;
    }

    double getX() const {
        return xPos;
    }

    double getY() const {
        return yPos;
    }

    double getXvel() const {
        return xVel;
    }

    double getYvel() const {
        return yVel;
    }

    double getRad() const {
        return radius;
    }

    double getMass() const {
        return mass;
    }

    public:
    bool getInfo() {
        const auto finite = [](double v) { return isfinite(v); };
        const auto positive = [](double v) { return isfinite(v) && v > 0; };
        Planet candidate = *this;
        if (!readNumber("Initial X position (m): ", candidate.xPos, finite, "Enter a finite number.") ||
            !readNumber("Initial Y position (m): ", candidate.yPos, finite, "Enter a finite number.") ||
            !readNumber("Initial X velocity (m/s): ", candidate.xVel, finite, "Enter a finite number.") ||
            !readNumber("Initial Y velocity (m/s): ", candidate.yVel, finite, "Enter a finite number.") ||
            !readNumber("Planet radius (m): ", candidate.radius, positive, "Enter a finite number greater than zero.") ||
            !readNumber("Planet mass (kg): ", candidate.mass, positive, "Enter a finite number greater than zero."))
            return false;
        *this = candidate;
        return true;
    }

    void printInfo() const {
        cout << "X pos: " << xPos << " (m)\n";
        cout << "Y pos: " << yPos << " (m)\n";
        cout << "X vel: " << xVel << " (m/s)\n";
        cout << "Y vel: " << yVel << " (m/s)\n";
        cout << "Radius: " << radius << "(m)\n";
        }

    void setAccel(double ax, double ay) {
        xAccel = ax;
        yAccel = ay;
    }

    void kick(double dt) {
        xVel += xAccel * dt;
        yVel += yAccel * dt;
    }
    void drift(double dt) {
        xPos += xVel * dt;
        yPos += yVel * dt;
    }
    void update(double dt) {
        // Exact motion when acceleration is constant over this interval.
        kick(dt * 0.5);
        drift(dt);
        kick(dt * 0.5);
    }

};

//______________________________________________________________________________
// PLANET SYSTEM: ASTEROIDS, SETUP AND GRAVITY
//Create class so user can choose particle #
class PlanetSystem {
    private:
    vector<Planet> planets;
    int asteroidCount = 0;

    void mergeOverlaps(vector<pair<size_t, size_t>>& merges) {
        // Restart after each merge: the new radius can overlap an earlier body.
        bool merged;
        do {
            merged = false;
            for (size_t i = 0; i < planets.size() && !merged; ++i) {
                for (size_t j = i + 1; j < planets.size(); ++j) {
                    const Planet& a = planets[i];
                    const Planet& b = planets[j];
                    if (hypot(b.getX() - a.getX(), b.getY() - a.getY()) >
                        a.getRad() + b.getRad()) continue;

                    const double mass = a.getMass() + b.getMass();
                    const double wa = a.getMass() / mass;
                    const double wb = b.getMass() / mass;
                    const bool blackHole = a.isBlackHole() || b.isBlackHole();
                    // Preserve volume for ordinary bodies; use the horizon for black holes.
                    const double scale = max(a.getRad(), b.getRad());
                    const double ra = a.getRad() / scale;
                    const double rb = b.getRad() / scale;
                    const double radius = blackHole ? 2 * G * mass / (299792458.0 * 299792458.0)
                        : scale * cbrt(ra * ra * ra + rb * rb * rb);
                    const Planet& appearance = a.isBlackHole() ? a : b.isBlackHole() ? b
                        : a.getMass() >= b.getMass() ? a : b;
                    Planet result(a.getX() * wa + b.getX() * wb,
                                  a.getY() * wa + b.getY() * wb,
                                  a.getXvel() * wa + b.getXvel() * wb,
                                  a.getYvel() * wa + b.getYvel() * wb,
                                  radius, mass, appearance.getColor(), blackHole,
                                  appearance.getName());
                    planets[i] = result;
                    planets.erase(planets.begin() + j);
                    merges.emplace_back(i, j);
                    merged = true;
                    break;
                }
            }
        } while (merged);
    }

    public:
    // ADD AN ASTEROID AT THE MOUSE POSITION
    void addAsteroid(sf::Vector2f mouse, const Camera& camera) {
        // Seed once, then choose new X/Y velocities (m/s) for each click.
        static mt19937 generator(random_device{}());
        uniform_real_distribution<double> velocity(-30000.0, 30000.0);

        // Reverse the camera projection so spawning works after panning or zooming.
        double x = camera.x + (mouse.x - 400.0) * SCALE / camera.zoom;
        double y = camera.y - (mouse.y - 300.0) * SCALE / camera.zoom;

        double vx = velocity(generator);
        double vy = velocity(generator);
        double radius = 1000.0;
        double mass = 1.0e12;

        asteroidCount++;
        string name = "Asteroid " + to_string(asteroidCount);
        sf::Color color(255, 165, 0);

        // The existing drawing code keeps small bodies visible at a 3-pixel radius.
        Planet asteroid(x, y, vx, vy, radius, mass, color, false, name);
        planets.push_back(asteroid);
    }

    // CHOOSE A PRESET OR ENTER PLANETS MANUALLY
    bool chooseSetup() {
        string answer;
        while (true) {
            cout << "Would you like to use a stellar preset? (yes/no): ";
            if (!getline(cin, answer)) return false;

            if (answer == "no" || answer == "n" || answer == "No" || answer == "N") {
                return getPlanets();
            }
            if (answer == "yes" || answer == "y" || answer == "Yes" || answer == "Y") {
                break;
            }
            cout << "Please enter yes or no.\n";
        }

        // Each preset stores a name and the bodies used to initialize the system.
        struct StellarPreset {
            string name;
            vector<Planet> bodies;
        };

        const double sunMass = 1.9885e30;
        const double earthMass = 5.972e24;
        const double separation = 1.496e11;
        const double orbitalSpeed = sqrt(G * (sunMass + earthMass) / separation);
        const double earthFraction = earthMass / (sunMass + earthMass);
        const double sunFraction = sunMass / (sunMass + earthMass);
        const double earthX = separation * sunFraction;
        const double earthSpeed = orbitalSpeed * sunFraction;
        const double moonDistance = 3.844e8;
        const double moonSpeed = sqrt(G * earthMass / moonDistance);

        // Reuse these starting bodies in the solar presets.
        const Planet sun(-separation * earthFraction, 0, 0,
                         -orbitalSpeed * earthFraction, 6.957e8, sunMass, sf::Color(255, 220, 80), false, "Sun");
        const Planet earth(earthX, 0, 0, earthSpeed, 6.371e6, earthMass, sf::Color(80, 160, 255), false, "Earth");

        // Equal stars are half the separation from their shared center.
        const double binarySpeed = sqrt(G * sunMass / (2 * separation));

        const double blackHoleMass = 10 * sunMass;
        const double c = 299792458.0;
        const double horizonRadius = 2 * G * blackHoleMass / (c * c);

        // Planet arguments: x, y, x velocity, y velocity, radius, mass (SI units).
        // Solar presets use approximate circular orbits, not date-specific positions.
        const vector<StellarPreset> presets = {
            {"Sun-Earth", {sun, earth}},
            {"Sun-Earth-Moon", {
                sun, earth,
                // The Moon shares Earth's motion, plus its own orbital velocity.
                Planet(earthX + moonDistance, 0, 0,
                       earthSpeed + moonSpeed, 1.7374e6, 7.342e22, sf::Color(210, 210, 210), false, "Moon")
            }},
            {"Mini solar system (Sun, Mercury, Venus, Earth, Mars)", {
                sun,
                // Circular speed = sqrt(G * central mass / orbital distance).
                Planet(5.791e10, 0, 0, sqrt(G * sunMass / 5.791e10), 2.4397e6, 3.301e23, sf::Color(160, 150, 140), false, "Mercury"),
                Planet(1.082e11, 0, 0, sqrt(G * sunMass / 1.082e11), 6.0518e6, 4.867e24, sf::Color(235, 190, 100), false, "Venus"),
                earth,
                Planet(2.279e11, 0, 0, sqrt(G * sunMass / 2.279e11), 3.3895e6, 6.417e23, sf::Color(225, 95, 65), false, "Mars")
            }},
            {"Binary stars (two Sun-like stars)", {
                Planet(-separation / 2, 0, 0, -binarySpeed, 6.957e8, sunMass, sf::Color(255, 200, 90), false, "Star A"),
                Planet( separation / 2, 0, 0,  binarySpeed, 6.957e8, sunMass, sf::Color(255, 245, 190), false, "Star B")
            }},
            {"Black hole flyby (Newtonian approximation)", {
                // This distant flyby stays well outside the relativistic region.
                // The black disk is an enlarged marker, not the true horizon size.
                Planet(0, 0, 0, 0, horizonRadius, blackHoleMass, sf::Color::Black, true, "Black hole"),
                Planet(-3.0e11, 1.5e11, 120000, 0, 6.371e6, earthMass,
                       sf::Color(80, 220, 255), false, "Passing body")
            }}
        };

        cout << "\nWhich preset would you like to use?\n";
        for (size_t i = 0; i < presets.size(); ++i) {
            cout << i + 1 << ". " << presets[i].name << '\n';
        }

        while (true) {
            cout << "Enter preset number: ";
            if (!getline(cin, answer)) return false;
            for (size_t i = 0; i < presets.size(); ++i) {
                if (answer == to_string(i + 1)) {
                    planets = presets[i].bodies;
                    cout << "Loaded " << presets[i].name << ".\n";
                    return true;
                }
            }
            cout << "Invalid preset. Choose a number from the list.\n";
        }
    }

    bool getPlanets() {
        int size{};
        // Bound manual allocation and the quadratic gravity workload.
        if (!readNumber("How many planets would you like in the sim (1-1000): ", size,
                        [](int count) { return count >= 1 && count <= 1000; },
                        "Enter a whole number from 1 to 1000.")) return false;
        vector<Planet> candidates(size);
        for (int i=0; i<size; i++) {
            cout << "\nPlanet " << i+1 << ":\n";
            if (!candidates[i].getInfo()) return false;
        }
        planets.swap(candidates);
        return true;
    }

    void printPlanet() const {
        cout << "You have " << planets.size() << " planets in the sim\n";

        for (int i=0; i<planets.size(); i++) {
            cout << "\n" << planets[i].getName() << ":\n";
            //By adding the [i] to particles class, allows us to look into each memeber of the class, which gives us access to the particle class containing the printInfo function
            planets[i].printInfo();
        }
    }

    private:
    void calculateAccelerations() {
        //Nested loop, claculate accel for every planet
        for (int i=0; i<planets.size(); i++) {

            double ax = 0.0;
            double ay = 0.0;

            for (int j=0; j<planets.size(); j++) {

                if (i==j) {
                    continue;
                }

                //Get x and y coord differences between planets
                double dx = planets[j].getX() - planets[i].getX();
                double dy = planets[j].getY() - planets[i].getY();

                //Compute distance between planets
                double distance = sqrt(dx*dx + dy*dy);

                if (distance == 0)
                    continue;

                //Compute force based on planets masses and distances from each other
                double F = G * (planets[i].getMass() * planets[j].getMass()) / (distance * distance);

                //Plnet i accel
                double x = dx/distance;
                double y = dy/distance;

                double accel = F/planets[i].getMass();

                ax += accel * x;
                ay += accel * y;
            }

            planets[i].setAccel(ax, ay);
        }

    }

    public:
    vector<pair<size_t, size_t>> update(double dt) {
        vector<pair<size_t, size_t>> merges;
        mergeOverlaps(merges);
        // Velocity Verlet: half kick, full drift, recompute gravity, half kick.
        calculateAccelerations();
        for (Planet& planet : planets) planet.kick(dt * 0.5);
        for (Planet& planet : planets) planet.drift(dt);
        // Merge before recalculating gravity to avoid forces inside overlapping bodies.
        // Mass-weighted half-step velocities preserve momentum across each merge.
        mergeOverlaps(merges);
        calculateAccelerations();
        for (Planet& planet : planets) planet.kick(dt * 0.5);
        return merges;
    }

    //VERY IMPORTANT LINE - GETTER FOR THE VECTOR and allows it to be reffered to by reference
    const vector<Planet>& getParSystem() {
    return planets;
    }
};

//______________________________________________________________________________
// GRID: CREATE LINES AND DISTORT THEM AROUND BODIES
//draws grid by incrementing x and y values and drawing lines between them rather than 1 long line
//Creates 2 vectors filled with vectors
class getGrid {
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

//______________________________________________________________________________
// ORBIT TRAILS
// Fixed-size history prevents trails from using unlimited memory.
struct OrbitTrail {
    static constexpr size_t capacity = 1024;
    array<sf::Vector2<double>, capacity> points{};
    size_t next = 0, count = 0;
    void add(const Planet& body) {
        points[next] = {body.getX(), body.getY()};
        next = (next + 1) % capacity;
        if (count < capacity) ++count;
    }
};

//______________________________________________________________________________
// VELOCITY DISPLAY
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

//______________________________________________________________________________
// MAIN: SETUP, MOUSE INPUT, PHYSICS AND DRAWING
int main() {

    //Create PlanetSystem class
    PlanetSystem p;
    getGrid g;
    Camera camera;

    //Initialize the values in the vector
    if (!p.chooseSetup()) return 0;
    p.printPlanet();
    cout << "Left-click: add asteroid | Right-drag: pan | Mouse wheel: zoom\n";

    // SET UP THE ORBIT TRAILS
    vector<OrbitTrail> trails(p.getParSystem().size());
    for (size_t i = 0; i < trails.size(); ++i) trails[i].add(p.getParSystem()[i]);
    vector<sf::Vertex> trailVertices((OrbitTrail::capacity + 1) * 2);
    array<sf::Vertex, 14> trailCapVertices{}; // center + 12 semicircle segments
    int trailStep = 0;

    // CREATE THE WINDOW AND LOAD THE FONT
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Gravity Sim");
    sf::Font font;
    if (!font.openFromFile(SIM_FONT_PATH)) {
        cerr << "Could not load the font: " << SIM_FONT_PATH << '\n';
        return 1;
    }
    VelocityDisplay velocityDisplay(font);

    // Reuse the shape instead of allocating its vertices for every body each frame.
    sf::CircleShape planet;
    SimulationTiming timing;
    sf::Clock frameClock;
    while (window.isOpen()) {
        // Advance existing bodies before input so a new asteroid is drawn at
        // its click position, without skipping time for the entire system.
        timing.advance(frameClock.restart().asMicroseconds(), [&](double dt) {
            for (const auto& merge : p.update(dt)) {
                // Keep trails aligned with the same sequential removals as the bodies.
                trails[merge.first] = OrbitTrail{};
                trails.erase(trails.begin() + merge.second);
            }
            if (++trailStep == 4) {
                for (size_t i = 0; i < trails.size(); ++i)
                    trails[i].add(p.getParSystem()[i]);
                trailStep = 0;
            }
        });

        //__________________________________________________________________________
        // MOUSE INPUT
        while (const auto event = window.pollEvent()) {
            if (const auto* button = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (button->button == sf::Mouse::Button::Left) {
                    p.addAsteroid(window.mapPixelToCoords(button->position), camera);
                    // Give the new body its own trail, just like preset/manual bodies.
                    trails.emplace_back();
                    trails.back().add(p.getParSystem().back());
                }
                if (button->button == sf::Mouse::Button::Right) {
                    camera.dragging = true;
                    camera.lastMouse = window.mapPixelToCoords(button->position);
                }
            }
            if (const auto* button = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (button->button == sf::Mouse::Button::Right) camera.dragging = false;
            }
            // Prevent a stuck drag when the mouse is released outside the window.
            if (event->is<sf::Event::FocusLost>() || event->is<sf::Event::MouseLeft>())
                camera.dragging = false;
            if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
                if (camera.dragging) camera.drag(window.mapPixelToCoords(mouse->position));
            }
            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->wheel == sf::Mouse::Wheel::Vertical)
                    camera.scroll(wheel->delta, window.mapPixelToCoords(wheel->position));
            }
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        if (!window.isOpen()) {
            break;
        }

        window.clear();

            //________________________________________________________________________
        //THIS DRAWS THE GRID
        g.updateGrid(p.getParSystem(), camera);

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
            const Planet& body = p.getParSystem()[i];
            sf::Color color = body.isBlackHole() ? sf::Color(180, 120, 255) : body.getColor();

            // Dim the trail's RGB values as well as its alpha, leaving the body bright.
            color.r = static_cast<unsigned char>(color.r * 0.55f);
            color.g = static_cast<unsigned char>(color.g * 0.55f);
            color.b = static_cast<unsigned char>(color.b * 0.55f);

            size_t oldest = (trail.next + OrbitTrail::capacity - trail.count) % OrbitTrail::capacity;
            float radius = body.getDisplayRadius();

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
        for (const Planet& body : p.getParSystem()) {

            // Fixed pixel radii keep small bodies visible without changing physics.
            float r = body.getDisplayRadius();
            auto position = camera.toScreen(body.getX(), body.getY());

            planet.setRadius(r);
            planet.setOrigin({r,r});
            planet.setFillColor(body.getColor());
            // Outline makes the black marker visible against the black background.
            planet.setOutlineThickness(body.isBlackHole() ? 2.0f : 0.0f);
            planet.setOutlineColor(sf::Color(180, 120, 255));

            planet.setPosition(position);

            window.draw(planet);
        }

        velocityDisplay.draw(window, p.getParSystem());

        //Displays the drawing
        window.display();
    }

    return 0;
}
