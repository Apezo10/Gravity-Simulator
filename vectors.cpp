#include "src/renderer.hpp"
#include "src/setup.hpp"
#include "src/simulation_timing.hpp"
#include <iostream>

using namespace std;

#ifndef SIM_FONT_PATH
#define SIM_FONT_PATH "assets/tuffy.ttf"
#endif

namespace {

    void followSelectedBody(const SimulationSession& session, Camera& camera) {


        if (session.following && session.selected) {
            const auto& body = session.system.getBodies()[*session.selected];
            camera.x = body.getX();
            camera.y = body.getY();
        }

    }

    void handleKey(sf::Keyboard::Key key, SimulationSession& session,
        Camera& camera, sf::Clock& frameClock) {


        switch (key) {
            case sf::Keyboard::Key::Space:
            session.paused = !session.paused;
            break;
            case sf::Keyboard::Key::Up:
            session.faster();
            break;
            case sf::Keyboard::Key::Down:
            session.slower();
            break;
            case sf::Keyboard::Key::R:
            session.reset();
            camera = Camera{
            };
            frameClock.restart();
            break;
            case sf::Keyboard::Key::F:


            if (session.selected) {
                session.following = !session.following;
            }

            break;
            case sf::Keyboard::Key::Escape:
            session.selected.reset();
            session.following = false;
            break;
            default:
            break;
        }

    }

    void handleEvent(const sf::Event& event, sf::RenderWindow& window,
        SimulationSession& session, Camera& camera,
        const Renderer& renderer, sf::Clock& frameClock) {


        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            handleKey(key->code, session, camera, frameClock);
        }


        if (const auto* button = event.getIf<sf::Event::MouseButtonPressed>()) {


            if (button->button == sf::Mouse::Button::Left) {
                const auto mouse = window.mapPixelToCoords(button->position);


                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
                    session.selected = renderer.pickBody(mouse, session.system.getBodies(), camera);


                    if (!session.selected) {
                        session.following = false;
                    }
                } else {
                    const auto world = camera.toWorld(mouse);
                    session.addAsteroid(world.x, world.y);
                }
            }


            if (button->button == sf::Mouse::Button::Right) {
                session.following = false;
                camera.dragging = true;
                camera.lastMouse = window.mapPixelToCoords(button->position);
            }
        }


        if (const auto* button = event.getIf<sf::Event::MouseButtonReleased>()) {


            if (button->button == sf::Mouse::Button::Right) {
                camera.dragging = false;
            }
        }


        // Prevent a stuck drag when the mouse is released outside the window.
        if (event.is<sf::Event::FocusLost>() || event.is<sf::Event::MouseLeft>())
        {
            camera.dragging = false;
        }


        if (const auto* mouse = event.getIf<sf::Event::MouseMoved>()) {


            if (camera.dragging) {
                camera.drag(window.mapPixelToCoords(mouse->position));
            }
        }


        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {


            if (wheel->wheel == sf::Mouse::Wheel::Vertical)
            {
                camera.scroll(wheel->delta, window.mapPixelToCoords(wheel->position));
            }
        }


        if (event.is<sf::Event::Closed>()) {
            window.close();
        }

    }

}

int main() {
    PlanetSystem initialSystem;
    Camera camera;


    if (!chooseSetup(initialSystem)) {
        return 0;
    }

    printPlanets(initialSystem);
    cout << "Space: pause | Up/Down: speed | R: reset | Shift-click: select | F: follow\n";
    SimulationSession session(initialSystem);

    // Load the font before constructing the renderer that uses it.
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Gravity Sim");
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    sf::Font font;


    if (!font.openFromFile(SIM_FONT_PATH)) {
        cerr << "Could not load the font: " << SIM_FONT_PATH << '\n';
        return 1;
    }

    Renderer renderer(font);

    sf::Clock frameClock;


    while (window.isOpen()) {

        // Advance existing bodies before input so a new asteroid is drawn at
        // its click position, without skipping time for the entire system.
        session.advance(frameClock.restart().asMicroseconds());
        followSelectedBody(session, camera);


        while (const auto event = window.pollEvent()) {
            handleEvent(*event, window, session, camera, renderer, frameClock);
        }


        if (!window.isOpen()) {
            break;
        }

        window.clear();

        followSelectedBody(session, camera);
        renderer.draw(window, session, camera);
        window.display();
    }

    return 0;
}
