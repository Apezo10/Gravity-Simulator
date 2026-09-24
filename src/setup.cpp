#include "setup.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;

namespace {

    // Read a whole line so invalid input cannot spill into the next field.
    template <typename T, typename Validator>
    bool readNumber(const char* prompt, T& value, Validator valid, const char* error) {
        string line;


        while (true) {
            cout << prompt;


            if (!getline(cin, line)) {
                return false;
            }

            istringstream input(line);
            T candidate{
            };


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

    bool getInfo(Planet& body) {
        const auto finite = [](double v) {
            return isfinite(v);
        };
        const auto positive = [](double v) {
            return isfinite(v) && v > 0;
        };
        double xPos{
        }, yPos{
        }, xVel{
        }, yVel{
        }, radius{
        }, mass{
        };


        if (!readNumber("Initial X position (m): ", xPos, finite, "Enter a finite number.") ||
            !readNumber("Initial Y position (m): ", yPos, finite, "Enter a finite number.") ||
            !readNumber("Initial X velocity (m/s): ", xVel, finite, "Enter a finite number.") ||
            !readNumber("Initial Y velocity (m/s): ", yVel, finite, "Enter a finite number.") ||
            !readNumber("Planet radius (m): ", radius, positive, "Enter a finite number greater than zero.") ||
            !readNumber("Planet mass (kg): ", mass, positive, "Enter a finite number greater than zero."))
        {
            return false;
        }

        body = Planet(xPos, yPos, xVel, yVel, radius, mass);
        return true;
    }

    void printInfo(const Planet& body) {
        cout << "X pos: " << body.getX() << " (m)\n";
        cout << "Y pos: " << body.getY() << " (m)\n";
        cout << "X vel: " << body.getXVelocity() << " (m/s)\n";
        cout << "Y vel: " << body.getYVelocity() << " (m/s)\n";
        cout << "Radius: " << body.getRadius() << "(m)\n";
    }

    bool getPlanets(PlanetSystem& system) {
        int size{
        };


        // Bound manual allocation and the quadratic gravity workload.
        if (!readNumber("How many planets would you like in the sim (1-1000): ", size,
            [](int count) {
                return count >= 1 && count <= 1000;
            },
            "Enter a whole number from 1 to 1000.")) {
            return false;
        }

        vector<Planet> candidates(size);


        for (int i=0; i<size; i++) {
            cout << "\nPlanet " << i+1 << ":\n";


            if (!getInfo(candidates[i])) {
                return false;
            }
        }

        system.setBodies(std::move(candidates));
        return true;
    }
}

bool chooseSetup(PlanetSystem& system) {
    string answer;


    while (true) {
        cout << "Would you like to use a stellar preset? (yes/no): ";


        if (!getline(cin, answer)) {
            return false;
        }


        if (answer == "no" || answer == "n" || answer == "No" || answer == "N") {
            return getPlanets(system);
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
        -orbitalSpeed * earthFraction, 6.957e8, sunMass, BodyColor(255, 220, 80), false, "Sun");
    const Planet earth(earthX, 0, 0, earthSpeed, 6.371e6, earthMass, BodyColor(80, 160, 255), false, "Earth");

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
                    earthSpeed + moonSpeed, 1.7374e6, 7.342e22, BodyColor(210, 210, 210), false, "Moon")
        }},
        {"Mini solar system (Sun, Mercury, Venus, Earth, Mars)", {
                sun,

                // Circular speed = sqrt(G * central mass / orbital distance).
                Planet(5.791e10, 0, 0, sqrt(G * sunMass / 5.791e10), 2.4397e6, 3.301e23, BodyColor(160, 150, 140), false, "Mercury"),
                Planet(1.082e11, 0, 0, sqrt(G * sunMass / 1.082e11), 6.0518e6, 4.867e24, BodyColor(235, 190, 100), false, "Venus"),
                earth,
                Planet(2.279e11, 0, 0, sqrt(G * sunMass / 2.279e11), 3.3895e6, 6.417e23, BodyColor(225, 95, 65), false, "Mars")
        }},
        {"Binary stars (two Sun-like stars)", {
                Planet(-separation / 2, 0, 0, -binarySpeed, 6.957e8, sunMass, BodyColor(255, 200, 90), false, "Star A"),
                Planet( separation / 2, 0, 0,  binarySpeed, 6.957e8, sunMass, BodyColor(255, 245, 190), false, "Star B")
        }},
        {"Black hole flyby (Newtonian approximation)", {

                // This distant flyby stays well outside the relativistic region.
                // The black disk is an enlarged marker, not the true horizon size.
                Planet(0, 0, 0, 0, horizonRadius, blackHoleMass, BodyColor(0, 0, 0), true, "Black hole"),
                Planet(-3.0e11, 1.5e11, 120000, 0, 6.371e6, earthMass,
                    BodyColor(80, 220, 255), false, "Passing body")
        }}
    };

    cout << "\nWhich preset would you like to use?\n";


    for (size_t i = 0; i < presets.size(); ++i) {
        cout << i + 1 << ". " << presets[i].name << '\n';
    }


    while (true) {
        cout << "Enter preset number: ";


        if (!getline(cin, answer)) {
            return false;
        }


        for (size_t i = 0; i < presets.size(); ++i) {


            if (answer == to_string(i + 1)) {
                system.setBodies(presets[i].bodies);
                cout << "Loaded " << presets[i].name << ".\n";
                return true;
            }
        }

        cout << "Invalid preset. Choose a number from the list.\n";
    }
}

void printPlanets(const PlanetSystem& system) {
    const auto& planets = system.getBodies();
    cout << "You have " << planets.size() << " planets in the sim\n";


    for (const Planet& body : planets) {
        cout << "\n" << body.getName() << ":\n";
        printInfo(body);
    }
}
