#include "planet_system.hpp"
#include <iostream>
int main() {
 PlanetSystem s;
 s.setBodies({Planet(-10,0,20,0,1,1),Planet(10,0,-20,0,1,1)});
 auto merges=s.update(1);
 std::cout << "Crossing bodies: merges=" << merges.size() << ", bodies=" << s.getBodies().size() << '\n';
}
