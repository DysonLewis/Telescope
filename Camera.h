#ifndef CAMERA_H
#define CAMERA_H

#include "Ray.h"
#include "Mirror.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class CameraSensor : public Mirror {
public:
    sf::Vector2f center;
    float width;
    float angle;
    sf::Color drawColor;
    std::vector<sf::Vector2f> hitPoints;
    int totalRaysTraced;
    int blockedRays;
    
    const float SENSOR_WIDTH_MM = 11.2f;
    const float SENSOR_HEIGHT_MM = 6.3f;
    const float SENSOR_DIAGONAL_MM = 12.85f;
    const float PIXEL_SIZE_MICRONS = 2.9f;

    CameraSensor(sf::Vector2f c, float w, float ang = 0.0f, const std::string& n = "Camera");

    std::string getType() const override;
    void clearHits();
    sf::Vector2f getStart() const;
    sf::Vector2f getEnd() const;
    Intersection intersect(const Ray& ray) const override;
    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override;
    
    float getFocusSpread() const;
    float getRMSSpotSize() const;
    float getEffectiveFocalLength(float primaryFocalLength) const;
    float getAngularResolutionArcsec(float effectiveFocalLength) const;
    float getFieldOfViewArcmin(float effectiveFocalLength) const;
};

#endif // CAMERA_H