#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
using namespace std;

//Universal constant
const double G = 6.67430e-11;

//Scale metres to pixals
const double SCALE = 2.0e9;

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
    
    public: 
    Planet() = default;

    Planet(double x, double y, double vx, double vy, double r, double m)
        : xPos(x), yPos(y), xVel(vx), yVel(vy), radius(r), mass(m) {}

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

        // Planet arguments: x, y, x velocity, y velocity, radius, mass (SI units).
        // These are approximate circular starting orbits, not date-specific positions.
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

int main() {
    const int counts[] = {2, 3, 5, 2};
    for (int choice = 1; choice <= 4; ++choice) {
        istringstream input("yes\n" + to_string(choice) + "\n");
        auto* old = cin.rdbuf(input.rdbuf());
        cin.clear();
        PlanetSystem system;
        bool ok = system.chooseSetup();
        cin.rdbuf(old);
        cin.clear();
        if (!ok || system.getParSystem().size() != counts[choice - 1]) return 1;
        for (int step = 0; step < 720; ++step) system.update();
        for (const auto& body : system.getParSystem()) {
            if (!isfinite(body.getX()) || !isfinite(body.getY())) return 2;
        }
        if (choice == 2) {
            const auto& bodies = system.getParSystem();
            double dx = bodies[2].getX() - bodies[1].getX();
            double dy = bodies[2].getY() - bodies[1].getY();
            double distance = sqrt(dx * dx + dy * dy);
            if (distance < 3e8 || distance > 5e8) return 3;
        }
    }
    cout << "All four presets passed selection and 30 simulated days.\n";
}
