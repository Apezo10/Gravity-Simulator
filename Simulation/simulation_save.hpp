#pragma once
#include "simulation_session.hpp"
#include <filesystem>

// Failures leave the live session and any previous save intact.
bool saveSimulation(const SimulationSession& session, const std::filesystem::path& path,
    std::string& error);
bool loadSimulation(SimulationSession& session, const std::filesystem::path& path,
    std::string& error);
