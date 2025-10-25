#include "Ray.h"
#include "Mirror.h"
#include "Optimizer.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>

const float PRIMARY_FOCAL_LENGTH = 400.0f;
const int NUM_RAYS = 80;

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

class Slider {
public:
    sf::RectangleShape track;
    sf::CircleShape handle;
    sf::Text label;
    sf::Text valueText;
    float minVal, maxVal, currentVal;
    bool isDragging;
    sf::Vector2f position;
    float width;

    Slider(float x, float y, float w, float min, float max, float init, const std::string& text, sf::Font& font) {
        position = sf::Vector2f(x, y);
        width = w;
        minVal = min;
        maxVal = max;
        currentVal = init;
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
        ss << std::fixed << std::setprecision(1) << currentVal;
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
            currentVal = minVal + ratio * (maxVal - minVal);
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
    std::vector<Ray> rays;
    sf::Vector2f offset;
    float scale;
    CameraSensor* camera; // Pointer to camera sensor for hit detection

    Scene(sf::Vector2f off, float sc) : offset(off), scale(sc), camera(nullptr) {}

    void addMirror(std::unique_ptr<Mirror> mirror) {
        // Track camera sensor separately
        if (mirror->getType() == "camera") {
            camera = dynamic_cast<CameraSensor*>(mirror.get());
        }
        mirrors.push_back(std::move(mirror));
    }

    void traceRay(Ray& ray, int maxBounces = 4) {
        for (int bounce = 0; bounce < maxBounces; bounce++) {
            Intersection closest;
            Mirror* hitMirror = nullptr;
            
            // Green rays (bounce 2+) only collide with camera sensor
            bool isGreenRay = (bounce >= 2);
            
            for (auto& mirror : mirrors) {
                // Skip non-camera mirrors for green rays
                if (isGreenRay && mirror->getType() != "camera") {
                    continue;
                }
                
                Intersection intersection = mirror->intersect(ray);
                if (intersection.hit && intersection.distance < closest.distance) {
                    closest = intersection;
                    hitMirror = mirror.get();
                }
            }

            if (closest.hit && hitMirror) {
                // Check if we hit the camera sensor
                if (hitMirror->getType() == "camera") {
                    ray.path.push_back(closest.point);
                    if (camera) {
                        camera->hitPoints.push_back(closest.point);
                    }
                    break; // Stop ray at camera
                }
                
                // Check if incoming ray (bounce 0) hit secondary first - mark for no draw
                if (bounce == 0 && hitMirror->getType() == "hyperbolic") {
                    ray.bounces = -1; // Special flag: don't draw this ray
                    break;
                }
                
                // Otherwise reflect off mirror
                ray.reflect(closest.point, closest.normal);
            } else {
                // No intersection, extend ray
                ray.extend(500.0f);
                break;
            }
        }
        
        // If max bounces reached, extend ray
        if (ray.bounces == maxBounces) {
            ray.extend(300.0f);
        }
    }

    void drawRay(sf::RenderWindow& window, const Ray& ray) const {
        // Don't draw rays that hit secondary mirror first (marked with bounces = -1)
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

    // UI Controls
    Slider sliderSecondaryX(50, 780, 400, 50, 450, 250, "Secondary X Position", font);
    Slider sliderSecondaryY(50, 830, 400, -20, 20, 0, "Secondary Y Position", font);
    Slider sliderCameraX(500, 780, 400, 510, 600, 540, "Camera X Position (behind primary)", font);
    Slider sliderCameraY(500, 830, 400, -30, 30, 0, "Camera Y Position", font);

    // Optimization buttons
    Button optimizeButton(950, 780, 150, 30, "Optimize (Scan)", font);
    Button fineOptimizeButton(950, 820, 150, 30, "Fine Optimize", font);

    bool isOptimizing = false;
    OptimizationResult lastOptResult;

    Scene scene(sf::Vector2f(100, 450), 1.0f);
    
    // Primary parabolic mirror with central hole (light collector)
    auto primary = std::make_unique<ParabolicMirror>(
        PRIMARY_FOCAL_LENGTH,  // focal length
        -150.0f,               // yMin
        150.0f,                // yMax
        500.0f,                // centerX
        "Primary",
        25.0f                  // hole radius for light to pass through
    );
    
    // Secondary hyperbolic mirror (convex, points back toward primary)
    // Positioned near the focal point of the primary
    auto secondary = std::make_unique<HyperbolicMirror>(
        250.0f,    // centerX (near primary focal point)
        0.0f,      // centerY
        30.0f,     // semi-major axis a
        25.0f,     // semi-minor axis b
        -50.0f,    // yMin
        50.0f,     // yMax
        true,      // use left branch (convex surface facing primary)
        "Secondary"
    );

    // Camera sensor BEHIND primary mirror (receives light through hole)
    auto camera = std::make_unique<CameraSensor>(
        sf::Vector2f(540.0f, 0.0f),  // position behind primary (x > primary centerX)
        40.0f,                        // width
        M_PI / 2.0f,                  // angle (vertical)
        "Camera"
    );

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

                // Check optimize buttons
                if (optimizeButton.contains(mousePos) && !isOptimizing) {
                    optimizeButton.setPressed(true);
                    isOptimizing = true;
                    
                    // Run optimization
                    lastOptResult = TelescopeOptimizer::optimizeSecondaryPosition(
                        scene.mirrors,
                        NUM_RAYS,
                        -50.0f,    // ray start X
                        -120.0f,   // ray Y min
                        120.0f,    // ray Y max
                        50.0f,     // scan X min (expanded range)
                        450.0f,    // scan X max
                        5.0f,      // scan X step
                        -20.0f,    // scan Y min
                        20.0f,     // scan Y max
                        5.0f       // scan Y step
                    );
                    
                    // Apply optimized position
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
                    
                    // Run fine optimization around current position
                    lastOptResult = TelescopeOptimizer::fineOptimize(
                        scene.mirrors,
                        NUM_RAYS,
                        -50.0f,    // ray start X
                        -120.0f,   // ray Y min
                        120.0f,    // ray Y max
                        sliderSecondaryX.getValue(),  // start from current position
                        sliderSecondaryY.getValue(),
                        30.0f,     // search radius
                        5.0f,      // initial step
                        30         // max iterations
                    );
                    
                    // Apply optimized position
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
            }
        }

        window.clear(sf::Color(20, 20, 30));

        // Update secondary mirror position from sliders
        HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
        if (secondaryMirror) {
            secondaryMirror->centerX = sliderSecondaryX.getValue();
            secondaryMirror->centerY = sliderSecondaryY.getValue();
        }

        // Update camera position from sliders
        CameraSensor* cameraSensor = dynamic_cast<CameraSensor*>(scene.mirrors[2].get());
        if (cameraSensor) {
            cameraSensor->center.x = sliderCameraX.getValue();
            cameraSensor->center.y = sliderCameraY.getValue();
        }

        // Clear previous frame's camera hits
        if (scene.camera) {
            scene.camera->clearHits();
        }

        // Trace rays (parallel rays coming from the left, simulating distant star)
        scene.rays.clear();
        for (int i = 0; i < NUM_RAYS; i++) {
            float h = -120.0f + i * (240.0f / (NUM_RAYS - 1));
            Ray ray(sf::Vector2f(-50.0f, h), sf::Vector2f(1.0f, 0.0f), sf::Color::Red);
            scene.traceRay(ray);
            scene.rays.push_back(ray);
        }

        // Draw scene
        for (const auto& mirror : scene.mirrors)
            mirror->draw(window, scene.offset, scene.scale);

        for (const auto& ray : scene.rays)
            scene.drawRay(window, ray);

        // Draw UI
        sliderSecondaryX.draw(window);
        sliderSecondaryY.draw(window);
        sliderCameraX.draw(window);
        sliderCameraY.draw(window);
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

        // Display camera hit statistics
        if (scene.camera) {
            std::stringstream ss;
            ss << "Camera Hits: " << scene.camera->hitPoints.size() << " / " << NUM_RAYS;
            float percentage = (100.0f * scene.camera->hitPoints.size()) / NUM_RAYS;
            ss << " (" << std::fixed << std::setprecision(1) << percentage << "%)";
            
            sf::Text stats(ss.str(), font, 16);
            stats.setFillColor(sf::Color::Cyan);
            stats.setPosition(20, 80);
            window.draw(stats);
        }

        // Display optimization results
        if (lastOptResult.maxHits > 0) {
            std::stringstream ss;
            ss << "Last Optimization: X=" << std::fixed << std::setprecision(1) << lastOptResult.bestSecondaryX
               << ", Y=" << lastOptResult.bestSecondaryY
               << " -> " << lastOptResult.maxHits << " hits (" 
               << std::setprecision(1) << lastOptResult.hitPercentage << "%)";
            
            sf::Text optStats(ss.str(), font, 14);
            optStats.setFillColor(sf::Color(100, 255, 100));
            optStats.setPosition(20, 110);
            window.draw(optStats);
        }

        // Show optimization status
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