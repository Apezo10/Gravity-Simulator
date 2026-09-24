#pragma once
#include <cstdint>
#include <string>

// Presentation metadata without a dependency on a graphics library.
struct BodyColor {
    std::uint8_t r, g, b;
    constexpr BodyColor(std::uint8_t red = 135, std::uint8_t green = 206,
        std::uint8_t blue = 235) : r(red), g(green), b(blue) {}
};
class Planet {
    double xPos{
    };
    double yPos{
    };
    double xVel{
    };
    double yVel{
    };
    double radius{
    };
    double xAccel{
        0};
    double yAccel{
        0};
    double mass{
    };
    BodyColor color{
        135, 206, 235};
    bool blackHole = false;
    std::string name;

public:
    Planet() = default;

    Planet(double x, double y, double vx, double vy, double r, double m,
        BodyColor tint = BodyColor(135, 206, 235), bool isBlackHole = false,
        std::string bodyName = "")
    : xPos(x), yPos(y), xVel(vx), yVel(vy), radius(r), mass(m),
    color(tint), blackHole(isBlackHole), name(bodyName) {}

    BodyColor getColor() const {
        return color;
    }

    bool isBlackHole() const {
        return blackHole;
    }

    std::string getName() const {


        if (!name.empty()) {
            return name;
        }


        if (blackHole) {
            return "Black hole";
        }


        if (mass >= 1.59e29) {
            return "Star";
        }


        if (mass >= 1e26) {
            return "Giant planet";
        }


        if (mass >= 1e24) {
            return "Terrestrial planet";
        }


        if (mass >= 1e23) {
            return "Small planet";
        }

        return "Moon or small body";
    }

    double getX() const {
        return xPos;
    }

    double getY() const {
        return yPos;
    }

    double getXVelocity() const {
        return xVel;
    }

    double getYVelocity() const {
        return yVel;
    }

    double getRadius() const {
        return radius;
    }

    double getMass() const {
        return mass;
    }

    void setAcceleration(double ax, double ay) {
        xAccel = ax;
        yAccel = ay;
    }

    void addAcceleration(double ax, double ay) {
        xAccel += ax;
        yAccel += ay;
    }

    void advanceVelocity(double dt) {
        xVel += xAccel * dt;
        yVel += yAccel * dt;
    }

    void advancePosition(double dt) {
        xPos += xVel * dt;
        yPos += yVel * dt;
    }

    void update(double dt) {

        // Exact motion when acceleration is constant over this interval.
        advanceVelocity(dt * 0.5);
        advancePosition(dt);
        advanceVelocity(dt * 0.5);
    }

};
