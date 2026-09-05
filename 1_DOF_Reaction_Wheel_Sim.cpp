#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
using namespace std;

//Global constants
double constexpr pi = 3.141592653;
double constexpr grav = 9.81;

class Pendulum {
    //Create and initialize variables
    private:
    double mass{};
    double length{};
    double thetaUser{};
    double theta{};
    double dt{0.001};
    double aa{};
    double inertia{};
    double av{};
    double time{0.0};

    //Create a local class to determine x and y coordinates of pendulum tip
    class EndCoordinates{
        private:
        double x{0.0};
        double y{0.0};

        public:
        void solveXY(double theta, double length) {
            
            x = length*sin(theta);
            y = length*cos(theta);
        }

        //getter for x and y
        double getX() {
        return x;
        }

        double getY() {
        return y;
        }
    };

    public:
    //Get user input for parameters
    void getInfo() {

        cout << "What is the length of the rod (meters): ";
        cin >> length;
        cout << "What is the mass of the rod (kg): ";
        cin >> mass;
        cout << "What is the starting angle of the rod (deg): ";
        cin >> thetaUser;

        theta = thetaUser * pi/180;
    }

    public:
    //Not needed yet but soon
    void getInertia() {
        inertia = (1.0/3.0)*(mass)*(length)*(length);
    }

    public:
    //Solve for angular accel
    void getAngularAccel() {
        aa = ((3.0*grav)/(2.0*length))*(sin(theta));
    }

    public:
    //Increment every values over a timestep
    void update() {
        //Get angular accel, then ntegrate angular velocity first over time, then angle
        getAngularAccel();

        av+= aa * dt;
        theta += av * dt;
        //Increment a time step
        time += dt;
        }

// Function to print the values of the pendulum
    void printValues() {
        //Initialize local class
        EndCoordinates xy;
        xy.solveXY(theta, length);
        double x = xy.getX();
        double y = xy.getY();

            cout << "\nAngle: " << theta * 180/pi;
            cout << "\nAnglular Velocity: " << av * 180/pi << "deg/s";
            cout << "\nAngular Accel: " << aa * 180/pi << "deg/s/s";
            cout << "\n\n Coordinates of tip: (" << x << "," << y << ")\n\n";
    }

    //Determine state of pendulum
    bool neverFall() {
        return abs(theta) < 1e-8;
    }

    bool hitFloor() {
        EndCoordinates xy;
        xy.solveXY(theta, length);

        return xy.getY() <= 1e-6;
    }

    //Get time elapsed
    double getTime()  {
        return time;
    }

};

int runSim() {
    Pendulum p;
    

    p.getInfo();

    if (p.neverFall()) {
        cout << "Your pendulum will NEVER fall!";
        return 0;
    }

    //Increment simulation over 10 seconds
    for (int i = 0; i < 10000; i++) {
        p.update();

        //Print at 20 Hz
        if (i % 50 == 0) {
         p.printValues();
            }

        //Check if it floor every iteration
        if (p.hitFloor()) {
            cout << "Pendulum hit the floor!\n";
            cout << "It took " << p.getTime() << " seconds to hit the floor";
            //End loop once it falls
            break;
            }
        }
        return 0;
    }  


int main() {
    runSim();
    return 0;
} 