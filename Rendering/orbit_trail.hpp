#pragma once
#include "planet.hpp"
#include <array>
#include <cstddef>
struct WorldPoint {
    double x{
    }, y{
    };
};
struct OrbitTrail {
    static constexpr std::size_t capacity = 1024;
    std::array<WorldPoint, capacity> points{
    };
    std::size_t next = 0, count = 0;
    void add(const Planet& body) {
        points[next] = {
            body.getX(), body.getY()};
        next = (next + 1) % capacity;


        if (count < capacity) {
            ++count;
        }
    }
};
