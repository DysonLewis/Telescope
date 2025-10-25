#ifndef MIRROR_H
#define MIRROR_H

#include "Ray.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>

// Abstract base class for all mirror types
class Mirror {
public:
    std::string name;
    bool isActive;
    
    Mirror(const std::string& n = "Mirror") : name(n), isActive(true) {}
    virtual ~Mirror() = default;
    
    virtual Intersection intersect(const Ray& ray) const = 0;
    virtual void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const = 0;
    
    // For serialization/deserialization (future use)
    virtual std::string getType() const = 0;
};

// Parabolic mirror: y^2 = 4fx where f is focal length
class ParabolicMirror : public Mirror {
public:
    float focalLength;
    float yMin, yMax;
    float centerX;
    float holeRadius; // Radius of central hole (for Cassegrain configuration)
    sf::Color drawColor;

    ParabolicMirror(float f, float ymin, float ymax, float cx = 400.0f, 
                    const std::string& n = "Parabolic", float holeR = 0.0f)
        : Mirror(n), focalLength(f), yMin(ymin), yMax(ymax), centerX(cx), 
          holeRadius(holeR), drawColor(sf::Color::White) {}

    std::string getType() const override { return "parabolic"; }

    float getX(float y) const {
        return centerX - y * y / (4.0f * focalLength);
    }

    sf::Vector2f getNormal(float y) const {
        // Parametric form: x = c - y²/(4f), so dx/dy = -y/(2f)
        // Tangent vector: (dx/dy, 1) = (-y/(2f), 1)
        // Normal (perpendicular): (1, y/(2f))
        float dxdy = -y / (2.0f * focalLength);
        sf::Vector2f normal(1.0f, dxdy);
        float mag = std::sqrt(normal.x * normal.x + normal.y * normal.y);
        return sf::Vector2f(normal.x / mag, -normal.y / mag);
    }

    Intersection intersect(const Ray& ray) const override {
        Intersection result;
        result.mirrorPtr = this;
        
        double ox = ray.origin.x, oy = ray.origin.y;
        double dx = ray.direction.x, dy = ray.direction.y;

        // Ray: (x, y) = (ox, oy) + t(dx, dy)
        // Surface: x = cx - y²/(4f)
        // Substitute: ox + t*dx = cx - (oy + t*dy)²/(4f)
        // Rearrange to quadratic in t
        double a = dy * dy / (4.0 * focalLength);
        double b = dx + oy * dy / (2.0 * focalLength);
        double c = ox - centerX + oy * oy / (4.0 * focalLength);

        double t = -1;

        if (std::abs(a) < EPSILON) {
            // Linear equation
            if (std::abs(b) > EPSILON) t = -c / b;
        } else {
            // Quadratic equation
            double discriminant = b * b - 4.0 * a * c;
            if (discriminant >= 0) {
                double sqrtDisc = std::sqrt(discriminant);
                double t1 = (-b - sqrtDisc) / (2.0 * a);
                double t2 = (-b + sqrtDisc) / (2.0 * a);
                if (t1 > EPSILON) t = t1;
                else if (t2 > EPSILON) t = t2;
            }
        }

        if (t > EPSILON) {
            // Newton-Raphson refinement for accuracy
            auto surfaceEq = [&](double tp) {
                double y = oy + tp * dy;
                return ox + tp * dx - (centerX - y * y / (4.0 * focalLength));
            };
            auto derivative = [&](double tp) {
                double y = oy + tp * dy;
                return dx + dy * y / (2.0 * focalLength);
            };
            
            for (int i = 0; i < 3; i++) {
                double f = surfaceEq(t);
                double fPrime = derivative(t);
                if (std::abs(fPrime) > EPSILON) {
                    t -= f / fPrime;
                }
            }

            double yHit = oy + t * dy;
            
            // Check if hit point is within mirror bounds and outside the hole
            if (yHit >= yMin - EPSILON && yHit <= yMax + EPSILON) {
                // Check if ray hits the central hole (passes through)
                if (holeRadius > 0.0f && std::abs(yHit) < holeRadius) {
                    // Ray passes through hole, no intersection
                    return result;
                }
                
                result.hit = true;
                result.point = sf::Vector2f(static_cast<float>(ox + t * dx),
                                            static_cast<float>(yHit));
                result.normal = getNormal(static_cast<float>(yHit));
                
                // Ensure normal points toward incoming ray
                double dot = dx * result.normal.x + dy * result.normal.y;
                if (dot > 0.0) {
                    result.normal = sf::Vector2f(-result.normal.x, -result.normal.y);
                }
                result.distance = static_cast<float>(t);
            }
        }

        return result;
    }

    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override {
        if (!isActive) return;
        
        // Draw parabola in two parts if there's a hole
        if (holeRadius > 0.0f) {
            // Upper part (above hole)
            sf::VertexArray upperPart(sf::LineStrip);
            int steps = 100;
            for (int i = 0; i <= steps; i++) {
                float y = holeRadius + i * (yMax - holeRadius) / steps;
                if (y <= yMax) {
                    float x = getX(y);
                    sf::Vector2f screenPos(offset.x + x * scale, offset.y - y * scale);
                    upperPart.append(sf::Vertex(screenPos, drawColor));
                }
            }
            window.draw(upperPart);
            
            // Lower part (below hole)
            sf::VertexArray lowerPart(sf::LineStrip);
            for (int i = 0; i <= steps; i++) {
                float y = yMin + i * (-holeRadius - yMin) / steps;
                if (y >= yMin) {
                    float x = getX(y);
                    sf::Vector2f screenPos(offset.x + x * scale, offset.y - y * scale);
                    lowerPart.append(sf::Vertex(screenPos, drawColor));
                }
            }
            window.draw(lowerPart);
            
            // Draw hole edges
            float yHoleTop = holeRadius;
            float yHoleBottom = -holeRadius;
            float xHoleTop = getX(yHoleTop);
            float xHoleBottom = getX(yHoleBottom);
            
            sf::Vertex holeEdges[] = {
                sf::Vertex(sf::Vector2f(offset.x + xHoleTop * scale, offset.y - yHoleTop * scale), sf::Color(100, 100, 100)),
                sf::Vertex(sf::Vector2f(offset.x + (xHoleTop - 30) * scale, offset.y - yHoleTop * scale), sf::Color(100, 100, 100)),
                
                sf::Vertex(sf::Vector2f(offset.x + xHoleBottom * scale, offset.y - yHoleBottom * scale), sf::Color(100, 100, 100)),
                sf::Vertex(sf::Vector2f(offset.x + (xHoleBottom - 30) * scale, offset.y - yHoleBottom * scale), sf::Color(100, 100, 100))
            };
            window.draw(holeEdges, 4, sf::Lines);
            
        } else {
            // Draw complete parabola
            sf::VertexArray parabola(sf::LineStrip);
            int steps = 200;
            for (int i = 0; i <= steps; i++) {
                float y = yMin + i * (yMax - yMin) / steps;
                float x = getX(y);
                sf::Vector2f screenPos(offset.x + x * scale, offset.y - y * scale);
                parabola.append(sf::Vertex(screenPos, drawColor));
            }
            window.draw(parabola);
        }
    }
};

// Flat (plane) mirror
class FlatMirror : public Mirror {
public:
    sf::Vector2f center;
    float angle; // in radians
    float size;
    sf::Color drawColor;

    FlatMirror(sf::Vector2f c, float ang, float s, const std::string& n = "Flat")
        : Mirror(n), center(c), angle(ang), size(s), drawColor(sf::Color::Magenta) {}

    std::string getType() const override { return "flat"; }

    void setAngle(float angleDegrees) {
        angle = angleDegrees * M_PI / 180.0f;
    }
    
    void setPosition(float x, float y) {
        center.x = x;
        center.y = y;
    }
    
    void setPosition(float x) {
        center.x = x;
    }

    sf::Vector2f getStart() const {
        float halfSize = size / 2.0f;
        float dx = halfSize * std::cos(angle);
        float dy = halfSize * std::sin(angle);
        return sf::Vector2f(center.x - dx, center.y - dy);
    }

    sf::Vector2f getEnd() const {
        float halfSize = size / 2.0f;
        float dx = halfSize * std::cos(angle);
        float dy = halfSize * std::sin(angle);
        return sf::Vector2f(center.x + dx, center.y + dy);
    }

    sf::Vector2f getNormal() const {
        // Normal perpendicular to mirror direction
        float nx = -std::sin(angle);
        float ny = std::cos(angle);
        return sf::Vector2f(nx, ny);
    }

    Intersection intersect(const Ray& ray) const override {
        Intersection result;
        result.mirrorPtr = this;
        
        sf::Vector2f start = getStart();
        sf::Vector2f end = getEnd();
        sf::Vector2f mirrorDir(end.x - start.x, end.y - start.y);
        
        float dx = ray.direction.x, dy = ray.direction.y;
        float mx = mirrorDir.x, my = mirrorDir.y;
        float denom = dx * my - dy * mx;

        if (std::abs(denom) > EPSILON) {
            sf::Vector2f diff(start.x - ray.origin.x, start.y - ray.origin.y);
            float t = (diff.x * my - diff.y * mx) / denom;
            float s = (diff.x * dy - diff.y * dx) / denom;

            // Check if intersection is within mirror segment (with small margin)
            if (t > EPSILON && s >= -0.05f && s <= 1.05f) {
                // Newton-Raphson refinement
                for (int i = 0; i < 2; i++) {
                    float yHit = ray.origin.y + t * dy;
                    float xHit = ray.origin.x + t * dx;
                    float sHit = ((xHit - start.x) * mx + (yHit - start.y) * my) / (mx * mx + my * my);
                    float xMirror = start.x + sHit * mx;
                    float yMirror = start.y + sHit * my;
                    float f = (xHit - xMirror) * (-my) + (yHit - yMirror) * mx;
                    float fPrime = -dx * my + dy * mx;
                    if (std::abs(fPrime) > EPSILON) {
                        t -= f / fPrime;
                    }
                }

                result.hit = true;
                result.point = ray.pointAt(t);
                result.normal = getNormal();
                
                // Ensure normal points toward incoming ray
                float dot = ray.direction.x * result.normal.x + ray.direction.y * result.normal.y;
                if (dot > 0) {
                    result.normal = -result.normal;
                }
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
    }
};

// Hyperbolic mirror: (x-cx)²/a² - (y-cy)²/b² = 1
class HyperbolicMirror : public Mirror {
public:
    float centerX, centerY;
    float a, b; // semi-major and semi-minor axes
    float yMin, yMax;
    bool useLeftBranch; // true for left branch, false for right branch
    sf::Color drawColor;

    HyperbolicMirror(float cx, float cy, float semiMajor, float semiMinor, 
                     float ymin, float ymax, bool leftBranch = false,
                     const std::string& n = "Hyperbolic")
        : Mirror(n), centerX(cx), centerY(cy), a(semiMajor), b(semiMinor),
          yMin(ymin), yMax(ymax), useLeftBranch(leftBranch), 
          drawColor(sf::Color(255, 150, 255)) {}

    std::string getType() const override { return "hyperbolic"; }

    float getX(float y) const {
        // Hyperbola: (x-cx)²/a² - (y-cy)²/b² = 1
        // Solve for x: x = cx ± a*sqrt(1 + (y-cy)²/b²)
        float yRel = y - centerY;
        float term = 1.0f + (yRel * yRel) / (b * b);
        if (term < 0) return centerX;
        float xOffset = a * std::sqrt(term);
        return centerX + (useLeftBranch ? -xOffset : xOffset);
    }

    sf::Vector2f getNormal(float y) const {
        // For hyperbola (x-cx)²/a² - (y-cy)²/b² = 1
        // Implicit differentiation: 2(x-cx)/a² - 2(y-cy)/(b²) * dy/dx = 0
        // dy/dx = (x-cx)*b² / ((y-cy)*a²)
        // dx/dy = (y-cy)*a² / ((x-cx)*b²)
        float x = getX(y);
        float yRel = y - centerY;
        float xRel = x - centerX;
        
        if (std::abs(xRel) < EPSILON) {
            // At vertex, normal is horizontal
            return useLeftBranch ? sf::Vector2f(-1.0f, 0.0f) : sf::Vector2f(1.0f, 0.0f);
        }
        
        float dxdy = (yRel * a * a) / (xRel * b * b);
        sf::Vector2f normal(1.0f, dxdy);
        float mag = std::sqrt(normal.x * normal.x + normal.y * normal.y);
        normal = sf::Vector2f(normal.x / mag, -normal.y / mag);
        
        // For left branch, flip normal direction
        if (useLeftBranch) {
            normal = -normal;
        }
        
        return normal;
    }

    Intersection intersect(const Ray& ray) const override {
        Intersection result;
        result.mirrorPtr = this;
        
        double ox = ray.origin.x - centerX;
        double oy = ray.origin.y - centerY;
        double dx = ray.direction.x;
        double dy = ray.direction.y;

        // Ray: (x, y) = (ox, oy) + t(dx, dy) relative to center
        // Hyperbola: x²/a² - y²/b² = 1
        // Substitute and rearrange to quadratic in t
        double A = (dx * dx) / (a * a) - (dy * dy) / (b * b);
        double B = 2.0 * ((ox * dx) / (a * a) - (oy * dy) / (b * b));
        double C = (ox * ox) / (a * a) - (oy * oy) / (b * b) - 1.0;

        double t = -1;

        if (std::abs(A) < EPSILON) {
            // Linear equation
            if (std::abs(B) > EPSILON) {
                t = -C / B;
            }
        } else {
            // Quadratic equation
            double discriminant = B * B - 4.0 * A * C;
            if (discriminant >= 0) {
                double sqrtDisc = std::sqrt(discriminant);
                double t1 = (-B - sqrtDisc) / (2.0 * A);
                double t2 = (-B + sqrtDisc) / (2.0 * A);
                
                // Choose correct intersection based on branch
                if (t1 > EPSILON && t2 > EPSILON) {
                    double x1 = ox + t1 * dx;
                    double x2 = ox + t2 * dx;
                    
                    if (useLeftBranch) {
                        t = (x1 < x2) ? t1 : t2;
                    } else {
                        t = (x1 > x2) ? t1 : t2;
                    }
                } else if (t1 > EPSILON) {
                    t = t1;
                } else if (t2 > EPSILON) {
                    t = t2;
                }
            }
        }

        if (t > EPSILON) {
            // Newton-Raphson refinement
            auto surfaceEq = [&](double tp) {
                double x = ox + tp * dx;
                double y = oy + tp * dy;
                return (x * x) / (a * a) - (y * y) / (b * b) - 1.0;
            };
            auto derivative = [&](double tp) {
                double x = ox + tp * dx;
                double y = oy + tp * dy;
                return 2.0 * ((x * dx) / (a * a) - (y * dy) / (b * b));
            };
            
            for (int i = 0; i < 3; i++) {
                double f = surfaceEq(t);
                double fPrime = derivative(t);
                if (std::abs(fPrime) > EPSILON) {
                    t -= f / fPrime;
                }
            }

            double yHit = oy + t * dy + centerY;
            if (yHit >= yMin - EPSILON && yHit <= yMax + EPSILON) {
                result.hit = true;
                result.point = sf::Vector2f(static_cast<float>(ox + t * dx + centerX),
                                            static_cast<float>(yHit));
                result.normal = getNormal(static_cast<float>(yHit));
                
                // Ensure normal points toward incoming ray
                double dot = dx * result.normal.x + dy * result.normal.y;
                if (dot > 0.0) {
                    result.normal = -result.normal;
                }
                result.distance = static_cast<float>(t);
            }
        }

        return result;
    }

    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override {
        if (!isActive) return;
        
        sf::VertexArray hyperbola(sf::LineStrip);
        int steps = 200;
        for (int i = 0; i <= steps; i++) {
            float y = yMin + i * (yMax - yMin) / steps;
            float x = getX(y);
            sf::Vector2f screenPos(offset.x + x * scale, offset.y - y * scale);
            hyperbola.append(sf::Vertex(screenPos, drawColor));
        }
        window.draw(hyperbola);
    }
};

// Camera sensor - acts as a detection surface (doesn't reflect)
class CameraSensor : public Mirror {
public:
    sf::Vector2f center;
    float width;
    float angle; // in radians
    sf::Color drawColor;
    std::vector<sf::Vector2f> hitPoints; // Store where rays hit

    CameraSensor(sf::Vector2f c, float w, float ang = 0.0f, const std::string& n = "Camera")
        : Mirror(n), center(c), width(w), angle(ang), drawColor(sf::Color::Cyan) {}

    std::string getType() const override { return "camera"; }

    void clearHits() { hitPoints.clear(); }

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
                // Camera doesn't reflect, so normal doesn't matter
            }
        }
        return result;
    }

    void draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const override {
        if (!isActive) return;
        
        sf::Vector2f start = getStart();
        sf::Vector2f end = getEnd();
        
        // Draw sensor line
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(offset.x + start.x * scale, offset.y - start.y * scale), drawColor),
            sf::Vertex(sf::Vector2f(offset.x + end.x * scale, offset.y - end.y * scale), drawColor)
        };
        window.draw(line, 2, sf::Lines);
        
        // Draw hit points
        for (const auto& hit : hitPoints) {
            sf::CircleShape dot(2);
            dot.setFillColor(sf::Color::Red);
            dot.setPosition(offset.x + hit.x * scale - 2, offset.y - hit.y * scale - 2);
            window.draw(dot);
        }
    }
};

#endif // MIRROR_H