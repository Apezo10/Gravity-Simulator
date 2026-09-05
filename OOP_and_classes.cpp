#include <iostream>
#include <cmath>
using namespace std;

class Point2d {

    private:
    double m_x{0.0};
    double m_y{0.0};

    public:
    Point2d(double x, double y) 
        : m_x{x}, m_y{y}
    {}

    public:
    Point2d()
    {}

    public:
    void print() {
        cout << "2d-point: (" << m_x << "," << m_y << ")\n";
    }

    public:
    double distanceTo(const Point2d& p) {
        double distance = sqrt((m_x - p.m_x) * (m_x - p.m_x) + (m_y - p.m_y)*(m_y - p.m_y));

        return distance;
    }
};

int main() {
    Point2d first{};
    Point2d second{ 3.0, 4.0 };

    first.print();
    second.print();

    std::cout << "Distance between two points: " << first.distanceTo(second) << '\n';

    return 0;
}