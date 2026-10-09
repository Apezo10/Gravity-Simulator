#include "body_markers.hpp"
#include "menu_cache.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void referenceLabel(sf::RenderTarget& target, const sf::Font& font,
    const std::string& value, sf::Vector2f position, unsigned size, sf::Color color) {
    sf::Text text(font, value, size);
    text.setPosition(position);
    text.setFillColor(color);
    target.draw(text);
}

void referenceButton(sf::RenderTarget& target, const sf::Font& font,
    const sf::FloatRect& bounds, const std::string& value, bool selected) {
    sf::RectangleShape box(bounds.size);
    box.setPosition(bounds.position);
    box.setFillColor(selected ? sf::Color(40, 79, 116) : sf::Color(29, 39, 57));
    box.setOutlineThickness(1);
    box.setOutlineColor(selected ? sf::Color(100, 193, 230) : sf::Color(57, 73, 97));
    target.draw(box);
    referenceLabel(target, font, value, bounds.position + sf::Vector2f(12, 10),
        18, sf::Color(225, 231, 242));
}

template <typename Work> double elapsedMs(Work work) {
    const auto start = std::chrono::steady_clock::now();
    work();
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
}

int main() {
    try {
        sf::Font font;
        require(font.openFromFile(SIM_FONT_PATH), "Test font did not load");
        sf::RenderTexture reference({800, 600}), cached({800, 600});
        MenuCache menu;
        BodyMarkers markers;
        const double masses[] = {1e12, 1e24, 1e25, 1e27, 1e30};
        // Exercise unchanged frames, edits, selection, and changes of menu mode.
        for (int frame : {0, 0, 1, 2, 2, 0}) {
            reference.clear(sf::Color(12, 18, 29));
            cached.clear(sf::Color(12, 18, 29));
            menu.beginFrame();
            const std::string title = frame == 0 ? "GRAVITY SIM" : "Custom system | 2 bodies added";
            const unsigned size = frame == 0 ? 32 : 23;
            referenceLabel(reference, font, title, {40, 25}, size, sf::Color::White);
            menu.label(cached, font, title, {40, 25}, size, sf::Color::White);
            for (int i = 0; i < 6; ++i) {
                const sf::FloatRect bounds({40.f, 100.f + i * 55.f},
                    {frame == 0 ? 720.f : 450.f, 40.f});
                const std::string value = frame == 0 ? "Sun-Earth-Moon" : (frame == 1 ? "1.5e11 |" : "");
                referenceButton(reference, font, bounds, value, i == frame);
                menu.button(cached, font, bounds, value, i == frame);
            }
            for (int mode = 0; mode < 3; ++mode) {
                for (int i = 0; i < 5; ++i) {
                    const Planet planet(0, 0, 0, 0, 1, masses[i], {}, mode == 1);
                    const float radius = displayRadius(planet) + (mode == 2 ? 5 : 0);
                    sf::CircleShape original(radius);
                    original.setOrigin({radius, radius});
                    original.setOutlineThickness(mode == 1 ? 2.f : (mode == 2 ? 1.5f : 0.f));
                    auto& reused = mode == 2 ? markers.selection(planet) : markers.body(planet);
                    const sf::Vector2f position(60.f + i * 140.f, 460.f + mode * 50.f);
                    const sf::Color color = mode == 2 ? sf::Color::Transparent : sf::Color(80, 160, 255);
                    const sf::Color outline = mode == 2 ? sf::Color::White : sf::Color(180, 120, 255);
                    for (auto* shape : {&original, &reused}) {
                        shape->setPosition(position);
                        shape->setFillColor(color);
                        shape->setOutlineColor(outline);
                    }
                    reference.draw(original);
                    cached.draw(reused);
                }
            }
            reference.display();
            cached.display();
            const auto expected = reference.getTexture().copyToImage();
            const auto actual = cached.getTexture().copyToImage();
            require(std::memcmp(expected.getPixelsPtr(), actual.getPixelsPtr(), 800 * 600 * 4) == 0,
                "Cached rendering changed pixels");
        }

        // Benchmark CPU preparation only; drawing and frame-cap waits are excluded.
        constexpr int iterations = 20000;
        sf::CircleShape original;
        const Planet planet(0, 0, 0, 0, 1, 1e25);
        volatile float result = 0;
        const double originalMs = elapsedMs([&] {
            for (int i = 0; i < iterations; ++i) {
                original.setRadius(displayRadius(planet));
                original.setOrigin({10, 10});
                original.setOutlineThickness(0);
                result = original.getLocalBounds().size.x;
            }
        });
        const double cachedMs = elapsedMs([&] {
            for (int i = 0; i < iterations; ++i)
                result = markers.body(planet).getLocalBounds().size.x;
        });
        std::cout << "Pixel equivalence passed; marker geometry preparation: "
            << originalMs << " ms original, " << cachedMs << " ms cached ("
            << iterations << " markers).\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
