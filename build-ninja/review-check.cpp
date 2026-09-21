#include <sstream>
#include <chrono>
#define main simulation_main
#include "../vectors.cpp"
#undef main

void setup(PlanetSystem& system, const std::string& value) {
    std::istringstream input(value);
    std::ostringstream output;
    auto* oldIn = std::cin.rdbuf(input.rdbuf());
    auto* oldOut = std::cout.rdbuf(output.rdbuf());
    std::cin.clear();
    system.chooseSetup();
    std::cin.rdbuf(oldIn);
    std::cout.rdbuf(oldOut);
    std::cin.clear();
}
int main() {
    for (int choice=1; choice<=5; ++choice) {
        PlanetSystem system;
        setup(system, "yes\n" + std::to_string(choice) + "\n");
        bool finite=true;
        for (int i=0; i<17532; ++i) {
            system.update();
            for (const auto& p:system.getParSystem())
                finite &= std::isfinite(p.getX()) && std::isfinite(p.getY());
        }
        std::cout << "Preset " << choice << " one simulated year finite=" << finite << '\n';
    }
    PlanetSystem zero;
    setup(zero,"no\n2\n0\n0\n0\n0\n1\n0\n1000\n0\n0\n0\n1\n1e12\n");
    zero.update();
    std::cout << "Zero mass after one step x=" << zero.getParSystem()[0].getX() << '\n';
    PlanetSystem invalid;
    setup(invalid,"no\n1\nabc\n");
    std::cout << "Invalid input accepted bodies=" << invalid.getParSystem().size() << " mass=" << invalid.getParSystem()[0].getMass() << '\n';
    PlanetSystem close;
    setup(close,"no\n2\n0\n0\n0\n0\n6.957e8\n1.9885e30\n1e6\n0\n0\n0\n1000\n1e12\n");
    close.update();
    std::cout << "Body inside Sun after one step speed=" << std::abs(close.getParSystem()[1].getXvel()) << " m/s\n";
    PlanetSystem timing;
    setup(timing,"no\n1\n0\n0\n1\n0\n1\n1\n");
    for(int i=0;i<30;++i) timing.update();
    std::cout << "30 frames at 1 m/s: x=" << timing.getParSystem()[0].getX() << '\n';
    for(int i=30;i<60;++i) timing.update();
    std::cout << "60 frames at 1 m/s: x=" << timing.getParSystem()[0].getX() << '\n';
    sf::Vector2f point(1,0), previousEdge(0,10);
    auto direction=point-previousEdge;
    auto normal=sf::Vector2f(-direction.y*10/std::hypot(direction.x,direction.y),direction.x*10/std::hypot(direction.x,direction.y));
    std::cout << "Straight horizontal trail normal=" << normal.x << ',' << normal.y << " expected=0,10\n";
}
