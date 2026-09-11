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

    struct StelarPreset {
        double x{};
        double y{};
        double vx{};
        double vy{};
        double radius{};
        sf::Color color{};
    };
    
    //VERY IMPORTANT LINE - GETTER FOR THE VECTOR and allows it to be reffered to by reference
    const vector<Planet>& getParSystem() {
    return planets;
    }
};

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



    sf::Vector2f distortPoints(float x, float y, const vector<Planet>& planets) {

        float dxTotal = 0.0f;
        float dyTotal = 0.0f;

        for (const Planet& p : planets) {

            float px = 400.0f + static_cast<float> (p.getX()/SCALE);
            float py = 300.0f - static_cast<float> (p.getY()/SCALE);

            float dx = px - x;
            float dy = py - y;

            float r = sqrt(dx*dx + dy*dy);

            r = max(r,10.0f);

            float massFactor = static_cast<float> (std::log1p(p.getMass() / 5.972e24)/std::log(2.0));
            float strength = 15.0f * massFactor / (1.0f + massFactor)/(1.0f + r/40.0f);

            dxTotal += strength * dx/r;
            dyTotal += strength * dy/r;
        }

        return {
            x + dxTotal,
            y + dyTotal
        };
    }

    void updateGrid(const vector<Planet>& planets) {
        VectorOfLinesV.clear();
        VectorOfLinesH.clear();

        iterateLinesV();
        iterateLinesH();

        for (auto& line : VectorOfLinesV) {
            for (auto& vertex : line) {
                vertex.position = distortPoints(vertex.position.x, vertex.position.y, planets);
            }
        }

        for (auto& line : VectorOfLinesH) {
            for (auto& vertex : line) {
                vertex.position = distortPoints(vertex.position.x, vertex.position.y, planets);
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

//Create PlanetSystem class
PlanetSystem p;
getGrid g;

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
    g.updateGrid(p.getParSystem());

    //Draw all the verticle lines
    for (const auto& line : g.getLinesV()) {
        window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
    }

    //Draw all the horizontal lines
    for (const auto& line : g.getLinesH()) {
        window.draw(line.data(), line.size(), sf::PrimitiveType::LineStrip);
    }

    //______________________________________________________________________________
    // GO THROUGH EVERY PLAET IN ParSystem and DRAW
    for (const Planet& Planet : p.getParSystem()) {

        float mass = static_cast<float> (Planet.getMass());

        int yellow[3] = {255,255,0};
        int blue[3] = {135,206,235};

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

