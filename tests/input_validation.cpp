// Exercise the real implementation without starting the graphical application.
#include "setup.hpp"
#include "custom_setup.hpp"
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
        {
            CustomSetup draft;
            PlanetSystem system;
            require(!draft.finish(system), "Empty GUI setup accepted");
            const std::array<std::string, 6> valid = {
                "-1.5e11", "0", "0", "2.98e4", "6.371e6", "5.972e24"};
            for (std::size_t field = 0; field < valid.size(); ++field) {
                for (const std::string bad : {"", "abc", "12junk", "1 2", "nan", "inf", "1e999"}) {
                    draft.fields = valid;
                    draft.fields[field] = bad;
                    require(!draft.addBody(), "Invalid GUI field accepted");
                    require(draft.bodies.empty(), "Invalid GUI draft partially added");
                    require(draft.activeField == field, "Invalid GUI field not focused");
                }
            }
            for (std::size_t field : {4u, 5u}) {
                for (const std::string bad : {"0", "-1"}) {
                    draft.fields = valid;
                    draft.fields[field] = bad;
                    require(!draft.addBody(), "Nonpositive GUI radius/mass accepted");
                }
            }
            draft.fields = valid;
            require(draft.addBody(), "Valid GUI body rejected");
            require(!draft.hasDraft(), "Added GUI draft was not cleared");
            draft.fields[0] = "1";
            require(!draft.finish(system), "Incomplete second GUI body accepted");
            require(system.getBodies().empty(), "Incomplete GUI setup published");
            draft.fields = valid;
            require(draft.finish(system), "GUI start did not add pending valid body");
            require(system.getBodies().size() == 2, "GUI setup lost a body");
            require(system.getBodies()[0].getX() == -1.5e11 &&
                system.getBodies()[0].getYVelocity() == 2.98e4, "GUI values changed");
            draft.bodies.resize(1000);
            draft.fields = valid;
            require(!draft.addBody(), "GUI body limit exceeded");
        }

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
