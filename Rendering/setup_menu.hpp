#pragma once
#include "setup.hpp"
#include <SFML/Graphics.hpp>

// Returns false when the window is closed before setup is complete.
bool showSetupMenu(sf::RenderWindow& window, const sf::Font& font, PlanetSystem& system);
