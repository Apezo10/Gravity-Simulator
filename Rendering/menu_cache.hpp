#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>

// Slots follow draw order; changes in menu mode update their contents in place.
class MenuCache {
    struct Label {
        sf::Text text;
        std::string value;
        explicit Label(const sf::Font& font) : text(font) {}
    };
    std::deque<Label> labels;
    std::deque<sf::RectangleShape> buttons;
    std::size_t labelIndex = 0, buttonIndex = 0;
public:
    void beginFrame() { labelIndex = buttonIndex = 0; }
    void label(sf::RenderTarget& target, const sf::Font& font, const std::string& value,
        sf::Vector2f position, unsigned size = 18,
        sf::Color color = sf::Color(225, 231, 242)) {
        if (labelIndex == labels.size()) labels.emplace_back(font);
        auto& cached = labels[labelIndex++];
        cached.text.setFont(font);
        if (cached.value != value) {
            cached.text.setString(value);
            cached.value = value;
        }
        cached.text.setCharacterSize(size);
        cached.text.setPosition(position);
        cached.text.setFillColor(color);
        target.draw(cached.text);
    }
    void button(sf::RenderTarget& target, const sf::Font& font, const sf::FloatRect& bounds,
        const std::string& value, bool selected = false) {
        if (buttonIndex == buttons.size()) buttons.emplace_back();
        auto& box = buttons[buttonIndex++];
        if (box.getSize() != bounds.size) box.setSize(bounds.size);
        box.setPosition(bounds.position);
        box.setFillColor(selected ? sf::Color(40, 79, 116) : sf::Color(29, 39, 57));
        if (box.getOutlineThickness() != 1) box.setOutlineThickness(1);
        box.setOutlineColor(selected ? sf::Color(100, 193, 230) : sf::Color(57, 73, 97));
        target.draw(box);
        label(target, font, value, bounds.position + sf::Vector2f(12.f, 10.f));
    }
};
