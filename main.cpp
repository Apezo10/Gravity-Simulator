#include "renderer.hpp"
#include "setup.hpp"
#include "simulation_timing.hpp"
#include <iostream>
#include <array>
#include "performance_stats.hpp"

using namespace std;

#ifndef SIM_FONT_PATH
#define SIM_FONT_PATH "Rendering/tuffy.ttf"
#endif

namespace {

    void aimLaunch(SimulationSession& session, const Camera& camera, sf::Vector2f mouse) {
        if (!session.launch) return;

        const auto start = camera.toScreen(session.launch->x, session.launch->y);
        session.aimLaunch(mouse.x - start.x, mouse.y - start.y);
    }

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
            case sf::Keyboard::Key::F4:
            session.showAccuracy = !session.showAccuracy;
            break;
            case sf::Keyboard::Key::F3:
            session.showPerformance = !session.showPerformance;
            break;
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
            camera.reset();
            frameClock.restart();
            break;
            case sf::Keyboard::Key::F:


            if (session.selected) {
                session.following = !session.following;
            }

            break;
            case sf::Keyboard::Key::Escape:
            if (session.launch) {
                session.cancelLaunch();
                frameClock.restart();
                break;
            }

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


        if (const auto* resized = event.getIf<sf::Event::Resized>()) {
            if (resized->size.x > 0 && resized->size.y > 0) {
                camera.resize(resized->size);
                window.setView(sf::View(sf::FloatRect(
                    {0.f, 0.f},
                    {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)})));
                session.cancelLaunch();
                frameClock.restart();
            }
        }

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
                    camera.dragging = false;
                    session.beginLaunch(world.x, world.y);
                }
            }


            if (button->button == sf::Mouse::Button::Right && !session.launch) {
                session.following = false;
                camera.dragging = true;
                camera.lastMouse = window.mapPixelToCoords(button->position);
            }
        }


        if (const auto* button = event.getIf<sf::Event::MouseButtonReleased>()) {

            if (button->button == sf::Mouse::Button::Left && session.launch) {
                aimLaunch(session, camera, window.mapPixelToCoords(button->position));
                session.finishLaunch();
                frameClock.restart();
            }


            if (button->button == sf::Mouse::Button::Right) {
                camera.dragging = false;
            }
        }


        // Prevent a stuck drag when the mouse is released outside the window.
        if (event.is<sf::Event::FocusLost>() || event.is<sf::Event::MouseLeft>())
        {
            camera.dragging = false;
            session.cancelLaunch();
            frameClock.restart();
        }


        if (const auto* mouse = event.getIf<sf::Event::MouseMoved>()) {

            aimLaunch(session, camera, window.mapPixelToCoords(mouse->position));


            if (camera.dragging) {
                camera.drag(window.mapPixelToCoords(mouse->position));
            }
        }


        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {


            if (wheel->wheel == sf::Mouse::Wheel::Vertical && !session.launch)
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
    auto previousFrame = std::chrono::steady_clock::now();


    while (window.isOpen()) {

        // Advance existing bodies before input so a new asteroid is drawn at
        // its click position, without skipping time for the entire system.
        const auto currentFrame = std::chrono::steady_clock::now();
        const double frameMs = std::chrono::duration<double, std::milli>(currentFrame - previousFrame).count();
        previousFrame = currentFrame;
        const auto elapsed = frameClock.restart().asMicroseconds();
        const double physicsMs = measureMilliseconds([&] { session.advance(elapsed); });
        followSelectedBody(session, camera);


        while (const auto event = window.pollEvent()) {
            handleEvent(*event, window, session, camera, renderer, frameClock);
        }


        if (!window.isOpen()) {
            break;
        }

        window.clear();

        followSelectedBody(session, camera);
        renderer.draw(window, session, camera, frameMs, physicsMs);
        window.display();
    }

    return 0;
}
