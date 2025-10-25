#ifndef RAY_H
#define RAY_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

const float EPSILON = 1e-6f;

struct Intersection {
    bool hit;
    sf::Vector2f point;
    sf::Vector2f normal;
    float distance;
    const void* mirrorPtr; // For identifying which mirror was hit
    
    Intersection() : hit(false), distance(std::numeric_limits<float>::max()), mirrorPtr(nullptr) {}
};

class Ray {
public:
    sf::Vector2f origin;
    sf::Vector2f direction;
    std::vector<sf::Vector2f> path;
    sf::Color color;
    int bounces;

    Ray(sf::Vector2f orig, sf::Vector2f dir, sf::Color col = sf::Color::Red)
        : origin(orig), direction(dir), color(col), bounces(0) {
        path.push_back(origin);
        normalizeDirection();
    }

    void normalizeDirection() {
        float mag = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (mag > EPSILON) {
            direction.x /= mag;
            direction.y /= mag;
        }
    }

    sf::Vector2f pointAt(float t) const {
        return sf::Vector2f(origin.x + t * direction.x, origin.y + t * direction.y);
    }

    void reflect(const sf::Vector2f& hitPoint, const sf::Vector2f& normal) {
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

    void extend(float length) {
        sf::Vector2f endPoint = pointAt(length);
        path.push_back(endPoint);
    }
};

// Newton-Raphson refinement for ray-surface intersection
// Refines parameter t to minimize distance to surface
template<typename SurfaceFunc, typename DerivFunc>
float newtonRaphsonRefinement(float t0, const Ray& ray, SurfaceFunc surfaceEq, DerivFunc derivative, int maxIter = 3) {
    float t = t0;
    for (int i = 0; i < maxIter; i++) {
        float f = surfaceEq(t);
        float fPrime = derivative(t);
        if (std::abs(fPrime) > EPSILON) {
            t -= f / fPrime;
        } else {
            break;
        }
    }
    return t;
}

#endif // RAY_H