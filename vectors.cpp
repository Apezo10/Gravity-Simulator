#include <iostream>
#include <vector>
#include <cmath>
#include <SFML/Graphics.hpp>
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

        //Nested loop (Every planet is eventually represented by planets[i])
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
            planets[i].update();
        }
    }
    
    //VERY IMPORTANT LINE - GETTER FOR THE VECTOR and allows it to be reffered to by reference
    const vector<Planet>& getParSystem() {
    return planets;
    }
};


int main() {

//Create PlanetSystem class
PlanetSystem p;

//Initialize the values in the vector
p.getPlanets();
p.printPlanet();

sf::RenderWindow window(sf::VideoMode({800, 600}), "Gravity Sim");

while (window.isOpen()) {

    while (const auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
    }

    p.update();

    window.clear();

        //________________________________________________________________________
    //THIS DRAWS THE GRID
    for (int x = 0; x < 800; x += 50) {

        sf::RectangleShape lineX;
        lineX.setSize({1.0f, 600.0f});
        lineX.setPosition({static_cast<float>(x),0.0f});

        window.draw(lineX);
    }

    for (int y = 0; y < 600; y += 50) {

        sf::RectangleShape lineY;
        lineY.setSize({800.0f, 1.0f});
        lineY.setPosition({0,static_cast<float>(y)});

        window.draw(lineY);
    }

    //______________________________________________________________________________
    // GO THROUGH EVERY PLAET IN ParSystem and DRAW
    for (const Planet& Planet : p.getParSystem()) {

        float mass = static_cast<float> (Planet.getMass());

        float r = 10.0f;
        if (mass > 1e29) {
            r = 30.0f;
        }
        float x = 400.0 + static_cast<float> (Planet.getX()/SCALE);
        float y = 300.0f - static_cast<float> (Planet.getY()/SCALE);

        sf::CircleShape planet(r);
        planet.setOrigin({r,r});
        planet.setFillColor(sf::Color(135, 206, 235));

        planet.setPosition({x,y});

        window.draw(planet);
        }

    //Displays the drawing
    window.display();
    }
return 0;
}

