#include "Mirror.h"
#include <cmath>

// Mirror base class
Mirror::Mirror(const std::string& n) : name(n), isActive(true) {}

// ParabolicMirror implementation
ParabolicMirror::ParabolicMirror(float f, float ymin, float ymax, float cx, 
                                 const std::string& n, float holeR)
    : Mirror(n), focalLength(f), yMin(ymin), yMax(ymax), centerX(cx), 
      holeRadius(holeR), drawColor(sf::Color::White) {}

std::string ParabolicMirror::getType() const { 
    return "parabolic"; 
}

float ParabolicMirror::getX(float y) const {
    return centerX - y * y / (4.0f * focalLength);
}

sf::Vector2f ParabolicMirror::getNormal(float y) const {
    float dxdy = -y / (2.0f * focalLength);
    sf::Vector2f normal(1.0f, dxdy);
    float mag = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    return sf::Vector2f(normal.x / mag, -normal.y / mag);
}

Intersection ParabolicMirror::intersect(const Ray& ray) const {
    Intersection result;
    result.mirrorPtr = this;
    
    double ox = ray.origin.x, oy = ray.origin.y;
    double dx = ray.direction.x, dy = ray.direction.y;

    double a = dy * dy / (4.0 * focalLength);
    double b = dx + oy * dy / (2.0 * focalLength);
    double c = ox - centerX + oy * oy / (4.0 * focalLength);

    double t = -1;

    if (std::abs(a) < EPSILON) {
        if (std::abs(b) > EPSILON) t = -c / b;
    } else {
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
        
        if (yHit >= yMin - EPSILON && yHit <= yMax + EPSILON) {
            if (holeRadius > 0.0f && std::abs(yHit) < holeRadius) {
                return result;
            }
            
            result.hit = true;
            result.point = sf::Vector2f(static_cast<float>(ox + t * dx),
                                        static_cast<float>(yHit));
            result.normal = getNormal(static_cast<float>(yHit));
            
            double dot = dx * result.normal.x + dy * result.normal.y;
            if (dot > 0.0) {
                result.normal = sf::Vector2f(-result.normal.x, -result.normal.y);
            }
            result.distance = static_cast<float>(t);
        }
    }

    return result;
}

void ParabolicMirror::draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const {
    if (!isActive) return;
    
    if (holeRadius > 0.0f) {
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

// FlatMirror implementation
FlatMirror::FlatMirror(sf::Vector2f c, float ang, float s, const std::string& n)
    : Mirror(n), center(c), angle(ang), size(s), drawColor(sf::Color::Magenta) {}

std::string FlatMirror::getType() const { 
    return "flat"; 
}

void FlatMirror::setAngle(float angleDegrees) {
    angle = angleDegrees * M_PI / 180.0f;
}

void FlatMirror::setPosition(float x, float y) {
    center.x = x;
    center.y = y;
}

void FlatMirror::setPosition(float x) {
    center.x = x;
}

sf::Vector2f FlatMirror::getStart() const {
    float halfSize = size / 2.0f;
    float dx = halfSize * std::cos(angle);
    float dy = halfSize * std::sin(angle);
    return sf::Vector2f(center.x - dx, center.y - dy);
}

sf::Vector2f FlatMirror::getEnd() const {
    float halfSize = size / 2.0f;
    float dx = halfSize * std::cos(angle);
    float dy = halfSize * std::sin(angle);
    return sf::Vector2f(center.x + dx, center.y + dy);
}

sf::Vector2f FlatMirror::getNormal() const {
    float nx = -std::sin(angle);
    float ny = std::cos(angle);
    return sf::Vector2f(nx, ny);
}

Intersection FlatMirror::intersect(const Ray& ray) const {
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

        if (t > EPSILON && s >= -0.05f && s <= 1.05f) {
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
            
            float dot = ray.direction.x * result.normal.x + ray.direction.y * result.normal.y;
            if (dot > 0) {
                result.normal = -result.normal;
            }
            result.distance = t;
        }
    }
    return result;
}

void FlatMirror::draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const {
    if (!isActive) return;
    
    sf::Vector2f start = getStart();
    sf::Vector2f end = getEnd();
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(offset.x + start.x * scale, offset.y - start.y * scale), drawColor),
        sf::Vertex(sf::Vector2f(offset.x + end.x * scale, offset.y - end.y * scale), drawColor)
    };
    window.draw(line, 2, sf::Lines);
}

// HyperbolicMirror implementation
HyperbolicMirror::HyperbolicMirror(float cx, float cy, float semiMajor, float semiMinor, 
                                   float ymin, float ymax, bool leftBranch,
                                   const std::string& n)
    : Mirror(n), centerX(cx), centerY(cy), a(semiMajor), b(semiMinor),
      yMin(ymin), yMax(ymax), useLeftBranch(leftBranch), 
      drawColor(sf::Color(255, 150, 255)) {}

std::string HyperbolicMirror::getType() const { 
    return "hyperbolic"; 
}

float HyperbolicMirror::getX(float y) const {
    float yRel = y - centerY;
    float term = 1.0f + (yRel * yRel) / (b * b);
    if (term < 0) return centerX;
    float xOffset = a * std::sqrt(term);
    return centerX + (useLeftBranch ? -xOffset : xOffset);
}

sf::Vector2f HyperbolicMirror::getNormal(float y) const {
    float x = getX(y);
    float yRel = y - centerY;
    float xRel = x - centerX;
    
    if (std::abs(xRel) < EPSILON) {
        return useLeftBranch ? sf::Vector2f(-1.0f, 0.0f) : sf::Vector2f(1.0f, 0.0f);
    }
    
    float dxdy = (yRel * a * a) / (xRel * b * b);
    sf::Vector2f normal(1.0f, dxdy);
    float mag = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    normal = sf::Vector2f(normal.x / mag, -normal.y / mag);
    
    if (useLeftBranch) {
        normal = -normal;
    }
    
    return normal;
}

Intersection HyperbolicMirror::intersect(const Ray& ray) const {
    Intersection result;
    result.mirrorPtr = this;
    
    double ox = ray.origin.x - centerX;
    double oy = ray.origin.y - centerY;
    double dx = ray.direction.x;
    double dy = ray.direction.y;

    double A = (dx * dx) / (a * a) - (dy * dy) / (b * b);
    double B = 2.0 * ((ox * dx) / (a * a) - (oy * dy) / (b * b));
    double C = (ox * ox) / (a * a) - (oy * oy) / (b * b) - 1.0;

    double t = -1;

    if (std::abs(A) < EPSILON) {
        if (std::abs(B) > EPSILON) {
            t = -C / B;
        }
    } else {
        double discriminant = B * B - 4.0 * A * C;
        if (discriminant >= 0) {
            double sqrtDisc = std::sqrt(discriminant);
            double t1 = (-B - sqrtDisc) / (2.0 * A);
            double t2 = (-B + sqrtDisc) / (2.0 * A);
            
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
            
            double dot = dx * result.normal.x + dy * result.normal.y;
            if (dot > 0.0) {
                result.normal = -result.normal;
            }
            result.distance = static_cast<float>(t);
        }
    }

    return result;
}

void HyperbolicMirror::draw(sf::RenderWindow& window, const sf::Vector2f& offset, float scale) const {
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