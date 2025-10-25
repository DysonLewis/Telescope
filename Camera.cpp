#include "Camera.h"
#include <cmath>

CameraSensor::CameraSensor(sf::Vector2f c, float w, float ang, const std::string& n)
    : Mirror(n), center(c), width(w), angle(ang), drawColor(sf::Color::Cyan), 
      totalRaysTraced(0), blockedRays(0) {}

std::string CameraSensor::getType() const { 
    return "camera"; 
}

void CameraSensor::clearHits() { 
    hitPoints.clear();
    totalRaysTraced = 0;
    blockedRays = 0;
}

sf::Vector2f CameraSensor::getStart() const {
    float halfWidth = width / 2.0f;
    float dx = halfWidth * std::cos(angle);
    float dy = halfWidth * std::sin(angle);
    return sf::Vector2f(center.x - dx, center.y - dy);
}

sf::Vector2f CameraSensor::getEnd() const {
    float halfWidth = width / 2.0f;
    float dx = halfWidth * std::cos(angle);
    float dy = halfWidth * std::sin(angle);
    return sf::Vector2f(center.x + dx, center.y + dy);
}

Intersection CameraSensor::intersect(const Ray& ray) const {
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

void CameraSensor::draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const {
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

float CameraSensor::getFocusSpread() const {
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

float CameraSensor::getRMSSpotSize() const {
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

float CameraSensor::getEffectiveFocalLength(float primaryFocalLength) const {
    return primaryFocalLength;
}

float CameraSensor::getAngularResolutionArcsec(float effectiveFocalLength) const {
    float pixelSizeMM = PIXEL_SIZE_MICRONS / 1000.0f;
    float angularResArcsec = (pixelSizeMM / effectiveFocalLength) * 206265.0f;
    return angularResArcsec;
}

float CameraSensor::getFieldOfViewArcmin(float effectiveFocalLength) const {
    float fovWidthArcmin = (SENSOR_WIDTH_MM / effectiveFocalLength) * 3437.75f;
    return fovWidthArcmin;
}