#define main simulation_main
#include "../vectors.cpp"
#undef main
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw runtime_error(message);
}

PlanetSystem circularSystem() {
    istringstream input("yes\n1\n");
    ostringstream output;
    auto* oldInput = cin.rdbuf(input.rdbuf());
    auto* oldOutput = cout.rdbuf(output.rdbuf());
    PlanetSystem system;
    const bool ok = system.chooseSetup();
    cin.rdbuf(oldInput);
    cout.rdbuf(oldOutput);
    cin.clear();
    require(ok, "Preset setup failed");
    return system;
}

double energy(const vector<Planet>& bodies) {
    double result = 0;
    for (const auto& body : bodies)
        result += 0.5 * body.getMass() *
                  (body.getXvel() * body.getXvel() + body.getYvel() * body.getYvel());
    return result - G * bodies[0].getMass() * bodies[1].getMass() /
        hypot(bodies[1].getX() - bodies[0].getX(), bodies[1].getY() - bodies[0].getY());
}

double orbitError(int steps) {
    auto system = circularSystem();
    const auto& initial = system.getParSystem();
    const double separation = initial[1].getX() - initial[0].getX();
    const double period = 2 * acos(-1.0) * sqrt(pow(separation, 3) /
        (G * (initial[0].getMass() + initial[1].getMass())));
    for (int i = 0; i < steps; ++i) system.update(period / steps);
    const auto& bodies = system.getParSystem();
    return hypot(bodies[1].getX() - bodies[0].getX() - separation,
                 bodies[1].getY() - bodies[0].getY()) / separation;
}

int main() {
    try {
        Planet body(0, 0, 2, -3, 1, 1);
        body.setAccel(4, -2);
        body.update(2);
        require(body.getX() == 12 && body.getY() == -10 &&
                body.getXvel() == 10 && body.getYvel() == -7,
                "Constant acceleration motion incorrect");

        auto system = circularSystem();
        const auto& bodies = system.getParSystem();
        const double initialEnergy = energy(bodies);
        const double separation = bodies[1].getX() - bodies[0].getX();
        const double momentumScale = bodies[1].getMass() * abs(bodies[1].getYvel());
        double maxEnergyError = 0, maxRadiusError = 0;
        for (int step = 0; step < 175320; ++step) {
            require(system.update(PHYSICS_STEP_SECONDS).empty(), "Circular orbit collided");
            maxEnergyError = max(maxEnergyError, abs((energy(bodies) - initialEnergy) / initialEnergy));
            maxRadiusError = max(maxRadiusError, abs(hypot(bodies[1].getX() - bodies[0].getX(),
                bodies[1].getY() - bodies[0].getY()) / separation - 1));
            const double px = bodies[0].getMass() * bodies[0].getXvel() + bodies[1].getMass() * bodies[1].getXvel();
            const double py = bodies[0].getMass() * bodies[0].getYvel() + bodies[1].getMass() * bodies[1].getYvel();
            require(hypot(px, py) / momentumScale < 1e-10, "Momentum drift too large");
        }
        require(maxEnergyError < 1e-9, "Ten-year energy error too large");
        require(maxRadiusError < 1e-6, "Ten-year orbital radius error too large");
        const double coarse = orbitError(200), fine = orbitError(400);
        require(fine < coarse / 3.5 && fine > coarse / 4.5,
                "Halving timestep did not give second-order convergence");
        cout << "Ten-year maximum relative energy error: " << maxEnergyError
             << "; radius error: " << maxRadiusError
             << "; convergence ratio: " << coarse / fine << '\n';
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
