#include "src/renderer.hpp"
#include "src/setup.hpp"
#include "src/simulation_timing.hpp"
#include <iostream>
using namespace std;

#ifndef SIM_FONT_PATH
#define SIM_FONT_PATH "assets/tuffy.ttf"
#endif

int main() {

    //Create PlanetSystem class
    PlanetSystem p;
    Camera camera;

    //Initialize the values in the vector
    if (!chooseSetup(p)) return 0;
    printPlanets(p);
    cout << "Left-click: add asteroid | Right-drag: pan | Mouse wheel: zoom\n";

    // SET UP THE ORBIT TRAILS
    vector<OrbitTrail> trails(p.getParSystem().size());
    for (size_t i = 0; i < trails.size(); ++i) trails[i].add(p.getParSystem()[i]);
    int trailStep = 0;

    // CREATE THE WINDOW AND LOAD THE FONT
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Gravity Sim");
    sf::Font font;
    if (!font.openFromFile(SIM_FONT_PATH)) {
        cerr << "Could not load the font: " << SIM_FONT_PATH << '\n';
        return 1;
    }
    Renderer renderer(font);

    SimulationTiming timing;
    sf::Clock frameClock;
    while (window.isOpen()) {
        // Advance existing bodies before input so a new asteroid is drawn at
        // its click position, without skipping time for the entire system.
        timing.advance(frameClock.restart().asMicroseconds(), [&](double dt) {
            for (const auto& merge : p.update(dt)) {
                // Keep trails aligned with the same sequential removals as the bodies.
                trails[merge.first] = OrbitTrail{};
                trails.erase(trails.begin() + merge.second);
            }
            if (++trailStep == 4) {
                for (size_t i = 0; i < trails.size(); ++i)
                    trails[i].add(p.getParSystem()[i]);
                trailStep = 0;
            }
        });

        //__________________________________________________________________________
        // MOUSE INPUT
        while (const auto event = window.pollEvent()) {
            if (const auto* button = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (button->button == sf::Mouse::Button::Left) {
                    const auto world = camera.toWorld(window.mapPixelToCoords(button->position));
                    p.addAsteroid(world.x, world.y);
                    // Give the new body its own trail, just like preset/manual bodies.
                    trails.emplace_back();
                    trails.back().add(p.getParSystem().back());
                }
                if (button->button == sf::Mouse::Button::Right) {
                    camera.dragging = true;
                    camera.lastMouse = window.mapPixelToCoords(button->position);
                }
            }
            if (const auto* button = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (button->button == sf::Mouse::Button::Right) camera.dragging = false;
            }
            // Prevent a stuck drag when the mouse is released outside the window.
            if (event->is<sf::Event::FocusLost>() || event->is<sf::Event::MouseLeft>())
                camera.dragging = false;
            if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
                if (camera.dragging) camera.drag(window.mapPixelToCoords(mouse->position));
            }
            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->wheel == sf::Mouse::Wheel::Vertical)
                    camera.scroll(wheel->delta, window.mapPixelToCoords(wheel->position));
            }
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        if (!window.isOpen()) {
            break;
        }

        window.clear();

        renderer.draw(window, p.getParSystem(), trails, camera);

        //Displays the drawing
        window.display();
    }

    return 0;
}
