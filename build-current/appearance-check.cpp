#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <string>
#include <SFML/Graphics.hpp>
using namespace std;

//Universal constant
const double G = 6.67430e-11;

//Scale metres to pixals
const double SCALE = 2.0e9;

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
    double dt{3600};
    double mass{};
    // One shared style rule is used for manual bodies, presets, and trails.
    struct Appearance {
        sf::Color color;
        float radius;
        bool blackHole = false;
    };

    Appearance appearance() const {
        const double c = 299792458.0;
        double horizon = 2 * G * mass / (c * c);
        // Mass alone cannot distinguish a star from a black hole.
        // A 1% tolerance accepts rounded horizon radii entered by the user.
        if (mass > 0 && radius > 0 && radius <= horizon * 1.01)
            return {sf::Color::Black, 30.0f, true};
        if (mass >= 1.59e29) return {sf::Color(255, 220, 80), 30.0f};
        if (mass >= 1e26) return {sf::Color(220, 170, 110), 16.0f};
        if (mass >= 1e24) return {sf::Color(80, 160, 255), 10.0f};
        if (mass >= 1e23) return {sf::Color(225, 130, 90), 6.0f};
        return {sf::Color(210, 210, 210), 3.0f};
    }

    public:
    Planet() = default;

    Planet(double x, double y, double vx, double vy, double r, double m)
        : xPos(x), yPos(y), xVel(vx), yVel(vy), radius(r), mass(m) {}

    sf::Color getColor() const { return appearance().color; }
    float getDisplayRadius() const { return appearance().radius; }
    bool isBlackHole() const { return appearance().blackHole; }
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
    void getInfo() {
      cout << "Initial X position (m): ";
      cin >> xPos;

      cout << "Initial Y position (m): ";
      cin >> yPos;

      cout << "Initial X velocity (m/s): ";
      cin >> xVel;

      cout << "Initial Y velocity (m/s): ";
      cin >> yVel;

      cout << "What is your planets radius: ";
      cin >> radius;

      cout << "What is your planets mass: ";
      cin >> mass;
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

    void update() {
        //Updates xy by integrating accel and velocity (accel first ALWAYS)
        xVel += xAccel * dt;
        yVel += yAccel * dt;

        xPos += xVel * dt;
        yPos += yVel * dt;
    }
    

};

//Create class so user can choose particle #
class PlanetSystem {
    private: 
    vector<Planet> planets;

    public:
    bool chooseSetup() {
        string answer;
        while (true) {
            cout << "Would you like to use a stellar preset? (yes/no): ";
            if (!getline(cin, answer)) return false;

            if (answer == "no" || answer == "n" || answer == "No" || answer == "N") {
                getPlanets();
                return true;
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
                         -orbitalSpeed * earthFraction, 6.957e8, sunMass);
        const Planet earth(earthX, 0, 0, earthSpeed, 6.371e6, earthMass);

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
                       earthSpeed + moonSpeed, 1.7374e6, 7.342e22)
            }},
            {"Mini solar system (Sun, Mercury, Venus, Earth, Mars)", {
                sun,
                // Circular speed = sqrt(G * central mass / orbital distance).
                Planet(5.791e10, 0, 0, sqrt(G * sunMass / 5.791e10), 2.4397e6, 3.301e23),
                Planet(1.082e11, 0, 0, sqrt(G * sunMass / 1.082e11), 6.0518e6, 4.867e24),
                earth,
                Planet(2.279e11, 0, 0, sqrt(G * sunMass / 2.279e11), 3.3895e6, 6.417e23)
            }},
            {"Binary stars (two Sun-like stars)", {
                Planet(-separation / 2, 0, 0, -binarySpeed, 6.957e8, sunMass),
                Planet( separation / 2, 0, 0,  binarySpeed, 6.957e8, sunMass)
            }},
            {"Black hole flyby (Newtonian approximation)", {
                // This distant flyby stays well outside the relativistic region.
                // The black disk is an enlarged marker, not the true horizon size.
                Planet(0, 0, 0, 0, horizonRadius, blackHoleMass),
                Planet(-3.0e11, 1.5e11, 120000, 0, 6.371e6, earthMass)
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

    void getPlanets() {
        int size{};

        cout << "How many planets would you like in the sim: ";

        cin >> size;

        //Consider invalid input
        while (size <= 0 || cin.fail()) {

            cin.clear();
            cin.ignore(1000, '\n');

            cout << "Invalid input. Try again: ";
            cin >> size;
        }

        planets.resize(size);

        //This function allows the user to enter the size of the vector storing the amount of particles
        for (int i=0; i<size; i++) {
            cout << "\nPlanet " << i+1 << ":\n";
            planets[i].getInfo();
        }   
    }

    void printPlanet() const {
        cout << "You have " << planets.size() << " planets in the sim\n";

        for (int i=0; i<planets.size(); i++) {
            cout << "\nPlanet " << i +1 << ":\n";
            //By adding the [i] to particles class, allows us to look into each memeber of the class, which gives us access to the particle class containing the printInfo function
            planets[i].printInfo();
        }
    }

    //Updates the x and y position of the planet based on velocity input
    void update() {

        //Nested loop, claculate accel for every planet
        for (int i=0; i<planets.size(); i++) {
            
            double ax = 0.0;
            double ay = 0.0;
            double totalAccel = 0.0;

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
    
        //Now update every planet
        for (int i = 0; i<planets.size(); i++) {
            planets[i].update();
        }
    }

    
    //VERY IMPORTANT LINE - GETTER FOR THE VECTOR and allows it to be reffered to by reference
    const vector<Planet>& getParSystem() {
    return planets;
    }
};

//draws grid by incrementing x and y values and drawing lines between them rather than 1 long line
//Creates 2 vectors filled with vectors
class getGrid {
    private:
    // Padding exceeds the maximum combined displacement, keeping endpoints offscreen.
    static constexpr float padding = 200.0f;
    static constexpr float maxDisplacement = 150.0f;
    vector<vector<sf::Vertex>> VectorOfLinesV;
    vector<vector<sf::Vertex>> VectorOfLinesH;


    public:
    void iterateLinesV() {

        //Nested loop to iterate through each of the verticle grid lines
        for (float j=-padding; j<=800.0f + padding; j+=50.0f) {

            vector<sf::Vertex> lineV;

            for (float i=-padding; i<=600.0f + padding; i+=5.0f) {
                lineV.push_back(sf::Vertex{{j, i}});
            }

            VectorOfLinesV.push_back(lineV);
        }
    }

    void iterateLinesH() {

        //Nested loop to iterate through each of the verticle grid lines
        for (float i=-padding; i<=600.0f + padding; i+=50.0f) {

            vector<sf::Vertex> lineH;

            for (float j=-padding; j<=800.0f + padding; j+=5.0f) {
                lineH.push_back(sf::Vertex{{j, i}});
            }

            VectorOfLinesH.push_back(lineH);
        }
    }



    sf::Vector2f distortPoints(float x, float y, const vector<Planet>& planets, const Camera& camera) {

        float dxTotal = 0.0f;
        float dyTotal = 0.0f;

        for (const Planet& p : planets) {

            auto screen = camera.toScreen(p.getX(), p.getY()); float px = screen.x;
            float py = screen.y;

            float dx = px - x;
            float dy = py - y;

            float r = sqrt(dx*dx + dy*dy);

            r = max(r,10.0f);

            // Restore the compact inward distortion; black holes get deeper wells.
            // This is a visual cue, not a relativistic spacetime calculation.
            double massFactor = log1p(max(0.0, p.getMass()) / 5.972e24) / log(2.0);
            double depth = 15.0 * massFactor / (1.0 + massFactor);
            if (p.isBlackHole()) depth *= 10.0;
            float strength = static_cast<float>(depth / (1.0 + r / 80.0));
            strength = min(strength, 0.9f * r);
            dxTotal += strength * dx/r;
            dyTotal += strength * dy/r;
        }

        // Bound the combined pull even when several massive bodies overlap.
        float displacement = sqrt(dxTotal * dxTotal + dyTotal * dyTotal);
        if (displacement > maxDisplacement) {
            dxTotal *= maxDisplacement / displacement;
            dyTotal *= maxDisplacement / displacement;
        }
        return {
            x + dxTotal,
            y + dyTotal
        };
    }

    void updateGrid(const vector<Planet>& planets, const Camera& camera) {
        // Allocate the grid once, then update each vertex in place.
        if (VectorOfLinesV.empty()) iterateLinesV();
        if (VectorOfLinesH.empty()) iterateLinesH();

        for (size_t line = 0; line < VectorOfLinesV.size(); ++line) {
            for (size_t point = 0; point < VectorOfLinesV[line].size(); ++point) {
                VectorOfLinesV[line][point].position =
                    distortPoints(line * 50.0f - padding, point * 5.0f - padding, planets, camera);
            }
        }
        for (size_t line = 0; line < VectorOfLinesH.size(); ++line) {
            for (size_t point = 0; point < VectorOfLinesH[line].size(); ++point) {
                VectorOfLinesH[line][point].position =
                    distortPoints(point * 5.0f - padding, line * 50.0f - padding, planets, camera);
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


int main() {
    Planet sun(0, 0, 0, 0, 6.957e8, 1.9885e30);
    Planet hole(0, 0, 0, 0, 29533, 1.9885e31);
    Planet massiveStar(0, 0, 0, 0, 6.957e8, 1.9885e31);
    Planet earth(0, 0, 0, 0, 6.371e6, 5.972e24);
    if (sun.isBlackHole() || massiveStar.isBlackHole() || !hole.isBlackHole()) return 1;
    if (sun.getColor() != sf::Color(255, 220, 80) ||
        hole.getColor() != sf::Color::Black ||
        earth.getColor() != sf::Color(80, 160, 255)) return 2;
    getGrid grid;
    Camera camera;
    auto ordinary = grid.distortPoints(500, 300, {sun}, camera);
    auto deep = grid.distortPoints(500, 300, {hole}, camera);
    if (!(deep.x < ordinary.x && deep.x > 400)) return 3;
    vector<Planet> bodies(20, hole);
    grid.updateGrid(bodies, camera);
    auto outside = [](sf::Vector2f p) {
        return p.x < 0 || p.x > 800 || p.y < 0 || p.y > 600;
    };
    for (const auto& line : grid.getLinesV())
        if (!outside(line.front().position) || !outside(line.back().position)) return 4;
    for (const auto& line : grid.getLinesH())
        if (!outside(line.front().position) || !outside(line.back().position)) return 5;
    const auto* storage = grid.getLinesV()[0].data();
    grid.updateGrid(bodies, camera);
    if (storage != grid.getLinesV()[0].data()) return 6;
    cout << "Appearance, inward distortion, hidden endpoints, and buffer reuse passed.\n";
}
