#ifndef CAMERA_H
#define CAMERA_H

#include "Ray.h"
#include "Mirror.h"  // NEED THIS for inheritance
#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <string>

// Camera sensor - NOW INHERITS FROM Mirror
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

    CameraSensor(sf::Vector2f c, float w, float ang = 0.0f, const std::string& n = "Camera")
        : Mirror(n), center(c), width(w), angle(ang), drawColor(sf::Color::Cyan), 
          totalRaysTraced(0), blockedRays(0) {}

    std::string getType() const override { return "camera"; }

    void clearHits() { 
        hitPoints.clear();
        totalRaysTraced = 0;
        blockedRays = 0;
    }

    sf::Vector2f getStart() const {
        float halfWidth = width / 2.0f;
        float dx = halfWidth * std::cos(angle);
        float dy = halfWidth * std::sin(angle);
        return sf::Vector2f(center.x - dx, center.y - dy);
    }

    sf::Vector2f getEnd() const {
        float halfWidth = width / 2.0f;
        float dx = halfWidth * std::cos(angle);
        float dy = halfWidth * std::sin(angle);
        return sf::Vector2f(center.x + dx, center.y + dy);
    }

    Intersection intersect(const Ray& ray) const override {
        Intersection result;
        result.mirrorPtr = this;
        
        sf::Vector2f start = getStart();
        sf::Vector2f end = getEnd();
        sf::Vector2f sensorDir(end.x - start.x, end.y - start.y);
        
        float dx = ray.direction.x, dy = ray.direction.y;
        float sx = sensorDir.x, sy = sensorDir.y;
        float denom = dx * sy - dy * sx;

        if (std::abs(denom) > EPSILON) {
            sf::Vector2f diff(start.x - ray.origin.x, start.y - ray.origin.y);
            float t = (diff.x * sy - diff.y * sx) / denom;
            float s = (diff.x * dy - diff.y * dx) / denom;

            if (t > EPSILON && s >= 0.0f && s <= 1.0f) {
                result.hit = true;
                result.point = ray.pointAt(t);
                result.distance = t;
            }
        }
        return result;
    }

    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override {
        if (!isActive) return;
        
        sf::Vector2f start = getStart();
        sf::Vector2f end = getEnd();
        
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(offset.x + start.x * scale, offset.y - start.y * scale), drawColor),
            sf::Vertex(sf::Vector2f(offset.x + end.x * scale, offset.y - end.y * scale), drawColor)
        };
        window.draw(line, 2, sf::Lines);
        
        for (const auto& hit : hitPoints) {
            sf::CircleShape dot(2);
            dot.setFillColor(sf::Color::Red);
            dot.setPosition(offset.x + hit.x * scale - 2, offset.y - hit.y * scale - 2);
            window.draw(dot);
        }
    }

    float getFocusSpread() const {
        if (hitPoints.size() < 2) return 0.0f;
        
        float centerY = 0.0f;
        for (const auto& hit : hitPoints) {
            centerY += hit.y;
        }
        centerY /= hitPoints.size();
        
        float maxSpread = 0.0f;
        for (const auto& hit : hitPoints) {
            float dist = std::abs(hit.y - centerY);
            if (dist > maxSpread) maxSpread = dist;
        }
        
        return maxSpread * 2.0f;
    }

    float getRMSSpotSize() const {
        if (hitPoints.size() < 2) return 0.0f;
        
        float centerX = 0.0f, centerY = 0.0f;
        for (const auto& hit : hitPoints) {
            centerX += hit.x;
            centerY += hit.y;
        }
        centerX /= hitPoints.size();
        centerY /= hitPoints.size();
        
        float sumSqDist = 0.0f;
        for (const auto& hit : hitPoints) {
            float dx = hit.x - centerX;
            float dy = hit.y - centerY;
            sumSqDist += dx * dx + dy * dy;
        }
        
        return std::sqrt(sumSqDist / hitPoints.size());
    }
    
    float getEffectiveFocalLength(float primaryFocalLength) const {
        return primaryFocalLength;
    }
    
    float getAngularResolutionArcsec(float effectiveFocalLength) const {
        float pixelSizeMM = PIXEL_SIZE_MICRONS / 1000.0f;
        float angularResArcsec = (pixelSizeMM / effectiveFocalLength) * 206265.0f;
        return angularResArcsec;
    }
    
    float getFieldOfViewArcmin(float effectiveFocalLength) const {
        float fovWidthArcmin = (SENSOR_WIDTH_MM / effectiveFocalLength) * 3437.75f;
        return fovWidthArcmin;
    }
};

#endif // CAMERA_H