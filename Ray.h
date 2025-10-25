#ifndef RAY_H
#define RAY_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>

const float EPSILON = 1e-6f;

struct Intersection {
    bool hit;
    sf::Vector2f point;
    sf::Vector2f normal;
    float distance;
    const void* mirrorPtr;
    
    Intersection();
};

class Ray {
public:
    sf::Vector2f origin;
    sf::Vector2f direction;
    std::vector<sf::Vector2f> path;
    sf::Color color;
    int bounces;

    Ray(sf::Vector2f orig, sf::Vector2f dir, sf::Color col = sf::Color::Red);
    
    void normalizeDirection();
    sf::Vector2f pointAt(float t) const;
    void reflect(const sf::Vector2f& hitPoint, const sf::Vector2f& normal);
    void extend(float length);
};

// Newton-Raphson refinement for ray-surface intersection
template<typename SurfaceFunc, typename DerivFunc>
float newtonRaphsonRefinement(float t0, const Ray& ray, SurfaceFunc surfaceEq, 
                              DerivFunc derivative, int maxIter = 3);

// Template implementation must be in header
template<typename SurfaceFunc, typename DerivFunc>
float newtonRaphsonRefinement(float t0, const Ray& ray, SurfaceFunc surfaceEq, 
                              DerivFunc derivative, int maxIter) {
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