#include "setup_menu.hpp"
#include "custom_setup.hpp"
#include <algorithm>

namespace {
constexpr sf::Vector2f canvas{800.f, 600.f};

void resizeMenu(sf::RenderWindow& window) {
    const auto size = window.getSize();
    if (!size.x || !size.y) return;
    sf::View view(sf::FloatRect({0.f, 0.f}, canvas));
    const float scale = std::min(size.x / canvas.x, size.y / canvas.y);
    const sf::Vector2f fraction(canvas.x * scale / size.x, canvas.y * scale / size.y);
    view.setViewport(sf::FloatRect({(1.f - fraction.x) / 2, (1.f - fraction.y) / 2}, fraction));
    window.setView(view);
}

void label(sf::RenderWindow& window, const sf::Font& font, const std::string& value,
           sf::Vector2f position, unsigned size = 18, sf::Color color = sf::Color(225, 231, 242)) {
    sf::Text text(font, value, size);
    text.setPosition(position);
    text.setFillColor(color);
    window.draw(text);
}

void button(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds,
            const std::string& value, bool selected = false) {
    sf::RectangleShape box(bounds.size);
    box.setPosition(bounds.position);
    box.setFillColor(selected ? sf::Color(40, 79, 116) : sf::Color(29, 39, 57));
    box.setOutlineThickness(1);
    box.setOutlineColor(selected ? sf::Color(100, 193, 230) : sf::Color(57, 73, 97));
    window.draw(box);
    label(window, font, value, bounds.position + sf::Vector2f(12.f, 10.f));
}

sf::FloatRect presetRect(std::size_t i) {
    return {{40.f, 145.f + static_cast<float>(i) * 57.f}, {720.f, 45.f}};
}
sf::FloatRect fieldRect(std::size_t i) {
    return {{310.f, 140.f + static_cast<float>(i) * 47.f}, {450.f, 37.f}};
}
const sf::FloatRect backRect({40.f, 475.f}, {135.f, 44.f});
const sf::FloatRect undoRect({190.f, 475.f}, {170.f, 44.f});
const sf::FloatRect addRect({375.f, 475.f}, {165.f, 44.f});
const sf::FloatRect startRect({555.f, 475.f}, {205.f, 44.f});
}

bool showSetupMenu(sf::RenderWindow& window, const sf::Font& font, PlanetSystem& system) {
    const auto& presets = stellarPresets();
    CustomSetup draft;
    bool custom = false;
    std::size_t selected = 0;
    bool replaceField = true;
    resizeMenu(window);

    const auto choose = [&]() {
        if (selected == presets.size()) {
            custom = true;
            return false;
        }
        system.setBodies(presets[selected].bodies);
        return true;
    };

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return false;
            }
            if (event->is<sf::Event::Resized>()) resizeMenu(window);
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (custom) {
                    if (key->code == sf::Keyboard::Key::Escape) custom = false;
                    if (key->code == sf::Keyboard::Key::Tab) {
                        draft.activeField = (draft.activeField + (key->shift ? 5 : 1)) % 6;
                        replaceField = true;
                    }
                    if (key->code == sf::Keyboard::Key::Enter) {
                        if (key->control) {
                            if (draft.finish(system)) return true;
                        } else {
                            draft.addBody();
                        }
                        replaceField = true;
                    }
                } else {
                    if (key->code == sf::Keyboard::Key::Down)
                        selected = (selected + 1) % (presets.size() + 1);
                    if (key->code == sf::Keyboard::Key::Up)
                        selected = (selected + presets.size()) % (presets.size() + 1);
                    if (key->code == sf::Keyboard::Key::Enter && choose()) return true;
                }
            }
            if (custom) {
                if (const auto* text = event->getIf<sf::Event::TextEntered>()) {
                    auto& field = draft.fields[draft.activeField];
                    if (text->unicode == 8) {
                        if (replaceField) field.clear();
                        else if (!field.empty()) field.pop_back();
                        replaceField = false;
                        draft.error.clear();
                    } else if (text->unicode >= 32 && text->unicode < 127) {
                        if (replaceField) field.clear();
                        if (field.size() < 48) field += static_cast<char>(text->unicode);
                        replaceField = false;
                        draft.error.clear();
                    }
                }
            }
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button != sf::Mouse::Button::Left) continue;
                const auto point = window.mapPixelToCoords(mouse->position);
                if (!custom) {
                    for (std::size_t i = 0; i <= presets.size(); ++i) {
                        if (presetRect(i).contains(point)) {
                            selected = i;
                            if (choose()) return true;
                            break;
                        }
                    }
                } else {
                    for (std::size_t i = 0; i < 6; ++i) {
                        if (fieldRect(i).contains(point)) {
                            draft.activeField = i;
                            replaceField = true;
                        }
                    }
                    if (backRect.contains(point)) custom = false;
                    if (undoRect.contains(point) && !draft.bodies.empty()) {
                        draft.bodies.pop_back();
                        draft.error.clear();
                    }
                    if (addRect.contains(point)) {
                        draft.addBody();
                        replaceField = true;
                    }
                    if (startRect.contains(point)) {
                        if (draft.finish(system)) return true;
                        replaceField = true;
                    }
                }
            }
        }

        window.clear(sf::Color(12, 18, 29));
        label(window, font, "GRAVITY SIM", {40.f, 25.f}, 32);
        if (!custom) {
            label(window, font, "Choose a starting system", {40.f, 76.f}, 23);
            label(window, font, "Click a scenario, or use Up / Down and Enter.", {40.f, 110.f}, 16);
            for (std::size_t i = 0; i <= presets.size(); ++i)
                button(window, font, presetRect(i),
                       i < presets.size() ? presets[i].name : "Custom system", selected == i);
            label(window, font, "Solar presets use approximate circular orbits.", {40.f, 520.f}, 16);
        } else {
            label(window, font, "Custom system  |  " + std::to_string(draft.bodies.size()) + " bodies added",
                  {40.f, 76.f}, 23);
            label(window, font, "SI units. Example: 1.5e11. Click or Tab to replace a field.", {40.f, 110.f}, 16);
            const char* names[] = {"X position (m)", "Y position (m)", "X velocity (m/s)",
                                   "Y velocity (m/s)", "Radius (m)", "Mass (kg)"};
            for (std::size_t i = 0; i < 6; ++i) {
                label(window, font, names[i], {40.f, 148.f + static_cast<float>(i) * 47.f});
                const auto& value = draft.fields[i];
                const std::string visible = value.size() > 30 ? "..." + value.substr(value.size() - 30) : value;
                button(window, font, fieldRect(i), visible +
                       (draft.activeField == i ? " |" : ""), draft.activeField == i);
            }
            label(window, font, draft.error, {40.f, 437.f}, 15, sf::Color(255, 160, 145));
            button(window, font, backRect, "Back");
            button(window, font, undoRect, "Remove last");
            button(window, font, addRect, "Add body");
            button(window, font, startRect, "Start simulation");
            label(window, font, "Enter: add body  |  Ctrl+Enter: start  |  Esc: back", {40.f, 536.f}, 16);
            label(window, font, "Start also adds the current draft if it contains any values.", {40.f, 562.f}, 15);
        }
        window.display();
    }
    return false;
}
