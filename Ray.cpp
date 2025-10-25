#include "Ray.h"
#include <cmath>

// Intersection implementation
Intersection::Intersection() 
    : hit(false), distance(std::numeric_limits<float>::max()), mirrorPtr(nullptr) {}

// Ray implementation
Ray::Ray(sf::Vector2f orig, sf::Vector2f dir, sf::Color col)
    : origin(orig), direction(dir), color(col), bounces(0) {
    path.push_back(origin);
    normalizeDirection();
}

void Ray::normalizeDirection() {
    float mag = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (mag > EPSILON) {
        direction.x /= mag;
        direction.y /= mag;
    }
}

sf::Vector2f Ray::pointAt(float t) const {
    return sf::Vector2f(origin.x + t * direction.x, origin.y + t * direction.y);
}

void Ray::reflect(const sf::Vector2f& hitPoint, const sf::Vector2f& normal) {
    path.push_back(hitPoint);
    
    // Compute reflection: d' = d - 2(d·n)n
    float dot = direction.x * normal.x + direction.y * normal.y;
    direction.x = direction.x - 2.0f * dot * normal.x;
    direction.y = direction.y - 2.0f * dot * normal.y;

    // Offset origin slightly to avoid self-intersection
    float offset = 1e-5f * (std::abs(hitPoint.x) + std::abs(hitPoint.y) + 1.0f);
    origin = hitPoint + normal * offset;

    // Update color based on bounce count
    bounces++;
    if (bounces == 1) color = sf::Color::Blue;
    else if (bounces == 2) color = sf::Color::Green;
    else if (bounces == 3) color = sf::Color::Yellow;
    else color = sf::Color(200, 200, 200, 180);
}

void Ray::extend(float length) {
    sf::Vector2f endPoint = pointAt(length);
    path.push_back(endPoint);
}