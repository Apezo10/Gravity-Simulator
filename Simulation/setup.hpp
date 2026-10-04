#pragma once
#include "planet_system.hpp"
bool chooseSetup(PlanetSystem& system);
void printPlanets(const PlanetSystem& system);


struct StellarPreset {
    std::string name;
    std::vector<Planet> bodies;
};
const std::vector<StellarPreset>& stellarPresets();
