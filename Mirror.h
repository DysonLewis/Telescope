#ifndef MIRROR_H
#define MIRROR_H

#include "Ray.h"
#include <SFML/Graphics.hpp>
#include <string>

// Abstract base class for all mirror types
class Mirror {
public:
    std::string name;
    bool isActive;
    
    Mirror(const std::string& n = "Mirror");
    virtual ~Mirror() = default;
    
    virtual Intersection intersect(const Ray& ray) const = 0;
    virtual void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const = 0;
    virtual std::string getType() const = 0;
};

// Parabolic mirror
class ParabolicMirror : public Mirror {
public:
    float focalLength;
    float yMin, yMax;
    float centerX;
    float holeRadius;
    sf::Color drawColor;

    ParabolicMirror(float f, float ymin, float ymax, float cx = 400.0f, 
                    const std::string& n = "Parabolic", float holeR = 0.0f);

    std::string getType() const override;
    float getX(float y) const;
    sf::Vector2f getNormal(float y) const;
    Intersection intersect(const Ray& ray) const override;
    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override;
};

// Flat mirror
class FlatMirror : public Mirror {
public:
    sf::Vector2f center;
    float angle;
    float size;
    sf::Color drawColor;

    FlatMirror(sf::Vector2f c, float ang, float s, const std::string& n = "Flat");

    std::string getType() const override;
    void setAngle(float angleDegrees);
    void setPosition(float x, float y);
    void setPosition(float x);
    sf::Vector2f getStart() const;
    sf::Vector2f getEnd() const;
    sf::Vector2f getNormal() const;
    Intersection intersect(const Ray& ray) const override;
    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override;
};

// Hyperbolic mirror
class HyperbolicMirror : public Mirror {
public:
    float centerX, centerY;
    float a, b;
    float yMin, yMax;
    bool useLeftBranch;
    sf::Color drawColor;

    HyperbolicMirror(float cx, float cy, float semiMajor, float semiMinor, 
                     float ymin, float ymax, bool leftBranch = false,
                     const std::string& n = "Hyperbolic");

    std::string getType() const override;
    float getX(float y) const;
    sf::Vector2f getNormal(float y) const;
    Intersection intersect(const Ray& ray) const override;
    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override;
};

#endif // MIRROR_H