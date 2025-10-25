#include "Ray.h"
#include "Mirror.h"
#include "Camera.h"
#include "Optimizer.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>

const float PRIMARY_FOCAL_LENGTH = 400.0f;
const int NUM_RAYS = 10000;

class Button {
public:
    sf::RectangleShape shape;
    sf::Text label;
    bool isPressed;

    Button(float x, float y, float w, float h, const std::string& text, sf::Font& font) {
        shape.setSize(sf::Vector2f(w, h));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(50, 120, 200));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);

        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(14);
        label.setFillColor(sf::Color::White);
        
        sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition(
            x + (w - textBounds.width) / 2.0f,
            y + (h - textBounds.height) / 2.0f - 2
        );

        isPressed = false;
    }

    bool contains(sf::Vector2f pos) {
        return shape.getGlobalBounds().contains(pos);
    }

    void setPressed(bool pressed) {
        isPressed = pressed;
        if (pressed) {
            shape.setFillColor(sf::Color(30, 80, 150));
        } else {
            shape.setFillColor(sf::Color(50, 120, 200));
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(label);
    }
};

class IncrementButton {
public:
    sf::CircleShape shape;
    sf::Text label;
    bool isHovered;
    
    IncrementButton(float x, float y, float radius, const std::string& text, sf::Font& font) {
        shape.setRadius(radius);
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(70, 140, 220));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(1);
        isHovered = false;
        
        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(16);
        label.setFillColor(sf::Color::White);
        label.setStyle(sf::Text::Bold);
        
        sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition(
            x + radius - textBounds.width / 2.0f,
            y + radius - textBounds.height / 2.0f - 3
        );
    }
    
    bool contains(sf::Vector2f pos) {
        sf::Vector2f center(shape.getPosition().x + shape.getRadius(), 
                           shape.getPosition().y + shape.getRadius());
        float dx = pos.x - center.x;
        float dy = pos.y - center.y;
        return (dx * dx + dy * dy) <= (shape.getRadius() * shape.getRadius());
    }
    
    void setHighlight(bool highlight) {
        isHovered = highlight;
        if (highlight) {
            shape.setFillColor(sf::Color(90, 170, 255));
        } else {
            shape.setFillColor(sf::Color(70, 140, 220));
        }
    }
    
    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(label);
    }
};

class Slider {
public:
    sf::RectangleShape track;
    sf::CircleShape handle;
    sf::Text label;
    sf::Text valueText;
    float minVal, maxVal, currentVal;
    float stepSize;
    bool isDragging;
    sf::Vector2f position;
    float width;

    Slider(float x, float y, float w, float min, float max, float init, const std::string& text, sf::Font& font, float step = 1.0f) {
        position = sf::Vector2f(x, y);
        width = w;
        minVal = min;
        maxVal = max;
        currentVal = init;
        stepSize = step;
        isDragging = false;

        track.setSize(sf::Vector2f(w, 4));
        track.setPosition(x, y);
        track.setFillColor(sf::Color(100, 100, 100));

        handle.setRadius(8);
        handle.setFillColor(sf::Color(50, 150, 250));
        handle.setOrigin(8, 8);
        updateHandlePosition();

        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(14);
        label.setFillColor(sf::Color::White);
        label.setPosition(x, y - 20);

        valueText.setFont(font);
        valueText.setCharacterSize(12);
        valueText.setFillColor(sf::Color(150, 200, 255));
        updateValueText();
    }

    void updateHandlePosition() {
        float ratio = (currentVal - minVal) / (maxVal - minVal);
        handle.setPosition(position.x + ratio * width, position.y + 2);
        updateValueText();
    }

    void updateValueText() {
        std::stringstream ss;
        if (stepSize <= 0.01f) {
            ss << std::fixed << std::setprecision(4) << currentVal;
        } else if (stepSize < 0.1f) {
            ss << std::fixed << std::setprecision(2) << currentVal;
        } else {
            ss << std::fixed << std::setprecision(1) << currentVal;
        }
        valueText.setString(ss.str());
        valueText.setPosition(position.x + width + 10, position.y - 8);
    }

    void handleMousePress(sf::Vector2f mousePos) {
        if (handle.getGlobalBounds().contains(mousePos)) isDragging = true;
    }

    void handleMouseRelease() {
        isDragging = false;
    }

    void handleMouseMove(sf::Vector2f mousePos) {
        if (isDragging) {
            float ratio = (mousePos.x - position.x) / width;
            ratio = std::max(0.0f, std::min(1.0f, ratio));
            float rawValue = minVal + ratio * (maxVal - minVal);
            
            currentVal = std::round(rawValue / stepSize) * stepSize;
            currentVal = std::max(minVal, std::min(maxVal, currentVal));
            
            updateHandlePosition();
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(track);
        window.draw(handle);
        window.draw(label);
        window.draw(valueText);
    }

    float getValue() const {
        return currentVal;
    }
};

class Scene {
public:
    std::vector<std::unique_ptr<Mirror>> mirrors;
    CameraSensor* camera;
    std::vector<Ray> rays;
    sf::Vector2f offset;
    float scale;

    Scene(sf::Vector2f off, float sc) : camera(nullptr), offset(off), scale(sc) {}

    void addMirror(std::unique_ptr<Mirror> mirror) {
        mirrors.push_back(std::move(mirror));
    }
    
    void addCamera(CameraSensor* cam) {
        camera = cam;
    }

    void traceRay(Ray& ray, int maxBounces = 4) {
        for (int bounce = 0; bounce < maxBounces; bounce++) {
            Intersection closest;
            Mirror* hitMirror = nullptr;
            
            bool isGreenRay = (bounce >= 2);
            
            for (auto& mirror : mirrors) {
                if (isGreenRay) continue;
                
                Intersection intersection = mirror->intersect(ray);
                if (intersection.hit && intersection.distance < closest.distance) {
                    closest = intersection;
                    hitMirror = mirror.get();
                }
            }
            
            if (camera) {
                Intersection cameraHit = camera->intersect(ray);
                if (cameraHit.hit && cameraHit.distance < closest.distance) {
                    closest = cameraHit;
                    hitMirror = nullptr;
                }
            }

            if (closest.hit) {
                if (hitMirror == nullptr) {
                    ray.path.push_back(closest.point);
                    if (camera) {
                        camera->hitPoints.push_back(closest.point);
                    }
                    break;
                }
                
                if (bounce == 0 && hitMirror->getType() == "hyperbolic") {
                    ray.bounces = -1;
                    if (camera) {
                        camera->blockedRays++;
                    }
                    break;
                }
                
                ray.reflect(closest.point, closest.normal);
            } else {
                ray.extend(500.0f);
                break;
            }
        }
        
        if (ray.bounces == maxBounces) {
            ray.extend(300.0f);
        }
        
        if (ray.bounces >= 0 && camera) {
            camera->totalRaysTraced++;
        }
    }

    void drawRay(sf::RenderWindow& window, const Ray& ray) const {
        if (ray.bounces < 0) return;
        
        for (size_t i = 0; i < ray.path.size() - 1; i++) {
            sf::Color segColor = (i == 0 ? sf::Color::Red
                                  : i == 1 ? sf::Color::Blue
                                  : i == 2 ? sf::Color::Green
                                  : sf::Color(200, 200, 200, 180));
            sf::Vertex line[] = {
                sf::Vertex(worldToScreen(ray.path[i]), segColor),
                sf::Vertex(worldToScreen(ray.path[i + 1]), segColor)
            };
            window.draw(line, 2, sf::Lines);
        }
    }

    sf::Vector2f worldToScreen(sf::Vector2f worldPos) const {
        return sf::Vector2f(offset.x + worldPos.x * scale, offset.y - worldPos.y * scale);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1400, 900), "Ritchey-Chrétien Telescope Ray Tracing");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
        if (!font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf"))
            if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) 
                return -1;

    Slider sliderSecondaryX(50, 800, 300, 50, 450, 250, "Secondary X Position (mm)", font, 0.001f);
    Slider sliderSecondaryY(50, 850, 300, -20, 20, 0, "Secondary Y Position (mm)", font, 0.001f);
    Slider sliderCameraX(500, 800, 400, 510, 600, 540, "Camera X Position (mm)", font, 1.0f);
    Slider sliderCameraY(500, 850, 400, -30, 30, 0, "Camera Y Position (mm)", font, 1.0f);

    // Fine control buttons for secondary mirror - positioned ABOVE sliders
    IncrementButton secXDecButton(50, 768, 12, "-", font);
    IncrementButton secXIncButton(90, 768, 12, "+", font);
    IncrementButton secYDecButton(50, 818, 12, "-", font);
    IncrementButton secYIncButton(90, 818, 12, "+", font);

    Button optimizeButton(950, 800, 150, 30, "Optimize (Scan)", font);
    Button fineOptimizeButton(950, 840, 150, 30, "Fine Optimize", font);

    bool isOptimizing = false;
    OptimizationResult lastOptResult;

    Scene scene(sf::Vector2f(100, 450), 1.0f);
    
    auto primary = std::make_unique<ParabolicMirror>(
        PRIMARY_FOCAL_LENGTH,
        -150.0f,
        150.0f,
        500.0f,
        "Primary",
        25.0f
    );
    
    auto secondary = std::make_unique<HyperbolicMirror>(
        250.0f,
        0.0f,
        30.0f,
        25.0f,
        -50.0f,
        50.0f,
        true,
        "Secondary"
    );

    auto camera = std::make_unique<CameraSensor>(
        sf::Vector2f(540.0f, 0.0f),
        40.0f,
        M_PI / 2.0f,
        "Camera"
    );

    // CRITICAL: Set scene.camera pointer BEFORE moving
    scene.camera = camera.get();

    scene.addMirror(std::move(primary));
    scene.addMirror(std::move(secondary));
    scene.addMirror(std::move(camera));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) 
                window.close();

            sf::Vector2f mousePos;
            if (event.type == sf::Event::MouseButtonPressed)
                mousePos = sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
            if (event.type == sf::Event::MouseMoved)
                mousePos = sf::Vector2f(event.mouseMove.x, event.mouseMove.y);

            if (event.type == sf::Event::MouseButtonPressed) {
                sliderSecondaryX.handleMousePress(mousePos);
                sliderSecondaryY.handleMousePress(mousePos);
                sliderCameraX.handleMousePress(mousePos);
                sliderCameraY.handleMousePress(mousePos);

                // Handle increment buttons for secondary mirror
                if (secXDecButton.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::max(sliderSecondaryX.minVal, 
                                                           sliderSecondaryX.currentVal - 0.001f);
                    sliderSecondaryX.updateHandlePosition();
                }
                if (secXIncButton.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::min(sliderSecondaryX.maxVal, 
                                                           sliderSecondaryX.currentVal + 0.001f);
                    sliderSecondaryX.updateHandlePosition();
                }
                if (secYDecButton.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::max(sliderSecondaryY.minVal, 
                                                           sliderSecondaryY.currentVal - 0.01f);
                    sliderSecondaryY.updateHandlePosition();
                }
                if (secYIncButton.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::min(sliderSecondaryY.maxVal, 
                                                           sliderSecondaryY.currentVal + 0.01f);
                    sliderSecondaryY.updateHandlePosition();
                }

                if (optimizeButton.contains(mousePos) && !isOptimizing) {
                    optimizeButton.setPressed(true);
                    isOptimizing = true;
                    
                    // Scan only in X axis, keep Y at current value
                    lastOptResult = TelescopeOptimizer::optimizeSecondaryPosition(
                        scene.mirrors,
                        scene.camera,
                        NUM_RAYS,
                        -50.0f,
                        -120.0f,
                        120.0f,
                        50.0f,     // Scan full X range
                        450.0f,
                        0.5f,      // 1mm steps in X
                        sliderSecondaryY.getValue(),  // Keep Y fixed at current value
                        sliderSecondaryY.getValue(),  // Same Y (no scan)
                        1.0f       // Y step (won't matter since min=max)
                    );
                    
                    sliderSecondaryX.currentVal = lastOptResult.bestSecondaryX;
                    sliderSecondaryY.currentVal = lastOptResult.bestSecondaryY;
                    sliderSecondaryX.updateHandlePosition();
                    sliderSecondaryY.updateHandlePosition();
                    
                    isOptimizing = false;
                    optimizeButton.setPressed(false);
                }

                if (fineOptimizeButton.contains(mousePos) && !isOptimizing) {
                    fineOptimizeButton.setPressed(true);
                    isOptimizing = true;
                    
                    // Fine optimization - search ±3mm around current position with 0.01mm precision
                    lastOptResult = TelescopeOptimizer::fineOptimize(
                        scene.mirrors,
                        scene.camera,
                        NUM_RAYS,
                        -50.0f,
                        -120.0f,
                        120.0f,
                        sliderSecondaryX.getValue(),
                        sliderSecondaryY.getValue(),  // Y stays fixed
                        3.0f,      // Search radius ±3mm in X only
                        0.1f,      // Start with 0.1mm steps
                        2500         // 
                    );
                    
                    sliderSecondaryX.currentVal = lastOptResult.bestSecondaryX;
                    sliderSecondaryY.currentVal = lastOptResult.bestSecondaryY;
                    sliderSecondaryX.updateHandlePosition();
                    sliderSecondaryY.updateHandlePosition();
                    
                    isOptimizing = false;
                    fineOptimizeButton.setPressed(false);
                }
            }
            if (event.type == sf::Event::MouseButtonReleased) {
                sliderSecondaryX.handleMouseRelease();
                sliderSecondaryY.handleMouseRelease();
                sliderCameraX.handleMouseRelease();
                sliderCameraY.handleMouseRelease();
            }
            if (event.type == sf::Event::MouseMoved) {
                sliderSecondaryX.handleMouseMove(mousePos);
                sliderSecondaryY.handleMouseMove(mousePos);
                sliderCameraX.handleMouseMove(mousePos);
                sliderCameraY.handleMouseMove(mousePos);
                
                // Highlight buttons on hover
                secXDecButton.setHighlight(secXDecButton.contains(mousePos));
                secXIncButton.setHighlight(secXIncButton.contains(mousePos));
                secYDecButton.setHighlight(secYDecButton.contains(mousePos));
                secYIncButton.setHighlight(secYIncButton.contains(mousePos));
            }
        }

        window.clear(sf::Color(20, 20, 30));

        HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
        if (secondaryMirror) {
            secondaryMirror->centerX = sliderSecondaryX.getValue();
            secondaryMirror->centerY = sliderSecondaryY.getValue();
        }

        CameraSensor* cameraSensor = dynamic_cast<CameraSensor*>(scene.mirrors[2].get());
        if (cameraSensor) {
            cameraSensor->center.x = sliderCameraX.getValue();
            cameraSensor->center.y = sliderCameraY.getValue();
        }

        if (scene.camera) {
            scene.camera->clearHits();
        }

        scene.rays.clear();
        for (int i = 0; i < NUM_RAYS; i++) {
            float h = -120.0f + i * (240.0f / (NUM_RAYS - 1));
            Ray ray(sf::Vector2f(-50.0f, h), sf::Vector2f(1.0f, 0.0f), sf::Color::Red);
            scene.traceRay(ray);
            scene.rays.push_back(ray);
        }

        for (const auto& mirror : scene.mirrors)
            mirror->draw(window, scene.offset, scene.scale);

        for (const auto& ray : scene.rays)
            scene.drawRay(window, ray);

        sliderSecondaryX.draw(window);
        sliderSecondaryY.draw(window);
        sliderCameraX.draw(window);
        sliderCameraY.draw(window);
        
        // Draw increment buttons
        secXDecButton.draw(window);
        secXIncButton.draw(window);
        secYDecButton.draw(window);
        secYIncButton.draw(window);
        
        optimizeButton.draw(window);
        fineOptimizeButton.draw(window);

        sf::Text title("Ritchey-Chrétien Telescope - Light passes through hole in primary to camera behind", font, 17);
        title.setFillColor(sf::Color::White);
        title.setPosition(20, 20);
        window.draw(title);

        sf::Text info("White: Primary (w/ hole)  |  Magenta: Secondary (Hyperbolic)  |  Cyan: Camera (behind)", font, 14);
        info.setFillColor(sf::Color(200, 200, 200));
        info.setPosition(20, 50);
        window.draw(info);

        if (scene.camera) {
            std::stringstream ss;
            ss << "Camera Hits: " << scene.camera->hitPoints.size() << " / " << scene.camera->totalRaysTraced;
            float percentage = scene.camera->totalRaysTraced > 0 ? 
                (100.0f * scene.camera->hitPoints.size()) / scene.camera->totalRaysTraced : 0.0f;
            ss << " (" << std::fixed << std::setprecision(1) << percentage << "%)";
            
            if (scene.camera->blockedRays > 0) {
                ss << "  |  Blocked: " << scene.camera->blockedRays;
            }
            
            sf::Text stats(ss.str(), font, 16);
            stats.setFillColor(sf::Color::Cyan);
            stats.setPosition(20, 80);
            window.draw(stats);
            
            if (scene.camera->hitPoints.size() >= 2) {
                std::stringstream focusSS;
                focusSS << "Focus Spread: " << std::fixed << std::setprecision(2) 
                       << scene.camera->getFocusSpread() << " mm  |  RMS Spot: " 
                       << scene.camera->getRMSSpotSize() << " mm";
                
                sf::Text focusStats(focusSS.str(), font, 14);
                focusStats.setFillColor(sf::Color(100, 255, 150));
                focusStats.setPosition(20, 105);
                window.draw(focusStats);
            }
            
            std::stringstream opticalSS;
            float effectiveFocalLength = PRIMARY_FOCAL_LENGTH * 2.5f;
            float angularResArcsec = scene.camera->getAngularResolutionArcsec(effectiveFocalLength);
            float fovArcmin = scene.camera->getFieldOfViewArcmin(effectiveFocalLength);
            
            opticalSS << "Sensor: " << scene.camera->SENSOR_WIDTH_MM << "×" 
                     << scene.camera->SENSOR_HEIGHT_MM << " mm  |  Pixel: " 
                     << scene.camera->PIXEL_SIZE_MICRONS << " μm  |  f_eff: " 
                     << std::fixed << std::setprecision(0) << effectiveFocalLength << " mm";
            
            sf::Text opticalSpec(opticalSS.str(), font, 13);
            opticalSpec.setFillColor(sf::Color(200, 200, 255));
            opticalSpec.setPosition(20, 130);
            window.draw(opticalSpec);
            
            std::stringstream angularSS;
            angularSS << "Angular Resolution: " << std::fixed << std::setprecision(2) 
                     << angularResArcsec << " arcsec/pixel  |  FOV: " 
                     << std::setprecision(2) << fovArcmin << " × " 
                     << std::setprecision(2) << (fovArcmin * scene.camera->SENSOR_HEIGHT_MM / scene.camera->SENSOR_WIDTH_MM)
                     << " arcmin";
            
            sf::Text angularStats(angularSS.str(), font, 13);
            angularStats.setFillColor(sf::Color(255, 200, 150));
            angularStats.setPosition(20, 150);
            window.draw(angularStats);
        }

        if (lastOptResult.maxHits > 0) {
            std::stringstream ss;
            ss << "Last Optimization: X=" << std::fixed << std::setprecision(2) << lastOptResult.bestSecondaryX
               << ", Y=" << lastOptResult.bestSecondaryY
               << " mm -> " << lastOptResult.maxHits << " hits (" 
               << std::setprecision(1) << lastOptResult.hitPercentage << "%)"
               << "  |  Focus: " << std::setprecision(2) << lastOptResult.focusSpread << " mm";
            
            sf::Text optStats(ss.str(), font, 13);
            optStats.setFillColor(sf::Color(100, 255, 100));
            optStats.setPosition(20, 175);
            window.draw(optStats);
        }

        if (isOptimizing) {
            sf::Text optimizingText("Optimizing...", font, 16);
            optimizingText.setFillColor(sf::Color::Yellow);
            optimizingText.setPosition(950, 750);
            window.draw(optimizingText);
        }

        window.display();
    }
    
    return 0;
}