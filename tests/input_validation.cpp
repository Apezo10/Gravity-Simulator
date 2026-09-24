// Exercise the real implementation without starting the graphical application.
#include "setup.hpp"
#include "simulation_timing.hpp"
#include "orbit_trail.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

using namespace std;
#include <stdexcept>

struct ConsoleInput {
    istringstream input;
    ostringstream output;
    streambuf* oldInput;
    streambuf* oldOutput;
    explicit ConsoleInput(const string& text)
    : input(text), oldInput(cin.rdbuf(input.rdbuf())),
    oldOutput(cout.rdbuf(output.rdbuf())) {
        cin.clear();
    }

    ~ConsoleInput() {
        cin.rdbuf(oldInput);
        cout.rdbuf(oldOutput);
        cin.clear();
    }
};

void require(bool condition, const char* message) {


    if (!condition) {
        throw runtime_error(message);
    }
}

int main() {


    try {
        const string body = "-1.5e11\n0\n0\n2.98e4\n6.371e6\n5.972e24\n";
        {
            ConsoleInput io("no\nabc\n0\n-1\n1.5\n1001\n999999999999999999999\n1 junk\n1\n" + body);
            PlanetSystem system;
            require(chooseSetup(system), "Count retry failed");
            require(system.getBodies().size() == 1, "Wrong count");
            const auto& p = system.getBodies()[0];
            require(p.getX() == -1.5e11 && p.getYVelocity() == 2.98e4, "Valid signed/scientific values changed");
        }

        const vector<string> fields = {
            "-1.5e11", "0", "0", "2.98e4", "6.371e6", "5.972e24"};


        for (size_t field = 0; field < fields.size(); ++field) {
            string input = "no\n1\n";


            for (size_t i = 0; i < fields.size(); ++i) {


                if (i == field) {
                    input += "\nabc\n12junk\n1 2\nnan\ninf\n-inf\n1e999\n";


                    if (i >= 4) {
                        input += "0\n-1\n";
                    }
                }

                input += "  " + fields[i] + "  \n";
            }

            ConsoleInput io(input);
            PlanetSystem system;
            require(chooseSetup(system), "Field retry failed");
            const auto& p = system.getBodies()[0];
            require(p.getX() == -1.5e11 && p.getY() == 0 && p.getXVelocity() == 0 &&
                p.getYVelocity() == 2.98e4 && p.getRadius() == 6.371e6 && p.getMass() == 5.972e24,
                "Invalid value accepted or fields shifted");
            system.update(PHYSICS_STEP_SECONDS);
            require(isfinite(p.getX()) && isfinite(p.getY()), "Valid manual body became nonfinite");
        }

        // EOF at every setup stage, including after a completed first body.
        vector<string> cancelled = {
            "", "yes\n", "no\n", "no\nabc\n"};
        string partial = "no\n2\n";


        for (const auto& field : fields) {
            cancelled.push_back(partial);
            partial += field + "\n";
        }

        cancelled.push_back(partial);


        for (const auto& input : cancelled) {
            ConsoleInput io(input);
            PlanetSystem system;
            require(!chooseSetup(system), "EOF did not cancel setup");
            require(system.getBodies().empty(), "Partial setup was published");
        }

        {
            ConsoleInput io("no\n1\n0\n0\n0\n0\n1\n0\n");
            PlanetSystem system;
            require(!chooseSetup(system), "Zero mass followed by EOF accepted");
        }

        const size_t counts[] = {
            2, 3, 5, 2, 2};


        for (int choice = 1; choice <= 5; ++choice) {
            ConsoleInput io("maybe\nyes\ninvalid\n" + to_string(choice) + "\n");
            PlanetSystem system;
            require(chooseSetup(system), "Preset selection failed");
            require(system.getBodies().size() == counts[choice-1], "Wrong preset count");


            for (int step = 0; step < 17532; ++step) {
                system.update(PHYSICS_STEP_SECONDS);


                for (const auto& p : system.getBodies())
                {
                    require(isfinite(p.getX()) && isfinite(p.getY()) &&
                        isfinite(p.getXVelocity()) && isfinite(p.getYVelocity()), "Preset became nonfinite");
                }
            }
        }

        cout << "Input validation, EOF cancellation, and five one-year preset checks passed.\n";
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
