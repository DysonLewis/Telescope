#include "Ray.h"
#include "Mirror.h"
#include "Camera.h"
#include "Optimizer.h"
#include "BatchOptimizer.h"
#include "ConfigBuilder.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>

const int NUM_RAYS = 100;

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
    float incrementValue;
    
    IncrementButton(float x, float y, float radius, const std::string& text, sf::Font& font, float incValue = 1.0f) {
        shape.setRadius(radius);
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(70, 140, 220));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(1);
        isHovered = false;
        incrementValue = incValue;
        
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
        ss << std::fixed << std::setprecision(5) << currentVal;
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
                // Ray didn't hit anything - extend it far to show it missed
                ray.extend(2000.0f);
                break;
            }
        }
        
        if (ray.bounces == maxBounces) {
            // Ray exceeded max bounces - extend it to show it's still going
            ray.extend(2000.0f);
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

void rebuildConfiguration(std::vector<OpticalConfig>& availableConfigs, int currentConfigIndex,
                         Scene& scene, float primaryCenterX, Slider& sliderSecondaryX, 
                         Slider& sliderSecondaryY) {
    // Clear existing mirrors
    scene.mirrors.clear();
    
    // Build telescope configuration
    ConfigBuilder::buildTelescopeFromConfig(availableConfigs[currentConfigIndex],
                                           scene.mirrors, scene.camera, primaryCenterX);
    
    // Recreate camera at the end of mirrors list
    auto newCamera = std::make_unique<CameraSensor>(
        sf::Vector2f(primaryCenterX + 40.0f, 0.0f), 
        11.2f,  // Use actual sensor width
        M_PI / 2.0f, "Camera"
    );
    scene.camera = newCamera.get();
    scene.addMirror(std::move(newCamera));
    
    // Update sliders to match loaded config
    HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
    if (secondaryMirror) {
        sliderSecondaryX.currentVal = secondaryMirror->centerX;
        sliderSecondaryY.currentVal = secondaryMirror->centerY;
        sliderSecondaryX.updateHandlePosition();
        sliderSecondaryY.updateHandlePosition();
    }
}

int main() {
    // Larger window to see the whole telescope system
    sf::RenderWindow window(sf::VideoMode(1800, 1000), "Cassegrain Telescope Ray Tracing");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
        if (!font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf"))
            if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) 
                return -1;

    // Load configurations from results CSV
    std::vector<OpticalConfig> availableConfigs;
    std::string configFile = "optimization_results.csv";
    availableConfigs = BatchOptimizer::loadResultsFromCSV(configFile);
    
    // If no results file, create default config
    if (availableConfigs.empty()) {
        std::cout << "No optimization results found. Using default configuration." << std::endl;
        OpticalConfig defaultConfig;
        defaultConfig.primaryDiameter = 300.0f;
        defaultConfig.secondaryDiameter = 100.0f;
        defaultConfig.primaryR = 1600.0f;
        defaultConfig.secondaryR = -600.0f;
        defaultConfig.primaryF = 800.0f;
        defaultConfig.secondaryF = -300.0f;
        defaultConfig.primaryK = -1.0f;
        defaultConfig.secondaryK = -3.5f;
        defaultConfig.mirrorSeparation = 450.0f;
        defaultConfig.systemFocalLength = 2000.0f;
        defaultConfig.bestSecondaryX = 250.0f;
        defaultConfig.bestSecondaryY = 0.0f;
        availableConfigs.push_back(defaultConfig);
    }
    
    int currentConfigIndex = 0;
    float primaryCenterX = 1400.0f;  // Moved to right side of screen

    // UI Controls - adjusted positions for larger window
    Slider sliderSecondaryX(50, 900, 300, -2000, 2000, 250, "Secondary X (mm)", font, 0.001f);
    Slider sliderSecondaryY(50, 950, 300, -20, 20, 0, "Secondary Y (mm)", font, 0.001f);
    
    Button prevConfigButton(450, 900, 80, 30, "< Prev", font);
    Button nextConfigButton(540, 900, 80, 30, "Next >", font);
    Button loadConfigButton(630, 900, 100, 30, "Load CSV", font);
    Button centerSecondaryButton(740, 900, 140, 30, "Center Secondary", font);

    // Coarse and fine increment buttons for X
    IncrementButton secXDecCoarse(50, 868, 12, "-", font, 1.0f);
    IncrementButton secXIncCoarse(70, 868, 12, "+", font, 1.0f);
    IncrementButton secXDecFine(90, 868, 10, "-", font, 0.001f);
    IncrementButton secXIncFine(110, 868, 10, "+", font, 0.001f);
    
    // Coarse and fine increment buttons for Y
    IncrementButton secYDecCoarse(50, 918, 12, "-", font, 0.1f);
    IncrementButton secYIncCoarse(70, 918, 12, "+", font, 0.1f);
    IncrementButton secYDecFine(90, 918, 10, "-", font, 0.001f);
    IncrementButton secYIncFine(110, 918, 10, "+", font, 0.001f);

    Button optimizeButton(1200, 900, 150, 30, "Optimize", font);
    Button fineOptimizeButton(1200, 940, 150, 30, "Fine Tune", font);

    bool isOptimizing = false;
    OptimizationResult lastOptResult;

    // Adjusted scene offset and scale for better viewing with primary on right
    Scene scene(sf::Vector2f(100, 500), 0.7f);
    
    // Build initial telescope configuration
    rebuildConfiguration(availableConfigs, currentConfigIndex, scene, primaryCenterX, 
                        sliderSecondaryX, sliderSecondaryY);

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

                // Config navigation
                if (prevConfigButton.contains(mousePos) && currentConfigIndex > 0) {
                    currentConfigIndex--;
                    rebuildConfiguration(availableConfigs, currentConfigIndex, scene, primaryCenterX,
                                       sliderSecondaryX, sliderSecondaryY);
                }
                
                if (nextConfigButton.contains(mousePos) && currentConfigIndex < (int)availableConfigs.size() - 1) {
                    currentConfigIndex++;
                    rebuildConfiguration(availableConfigs, currentConfigIndex, scene, primaryCenterX,
                                       sliderSecondaryX, sliderSecondaryY);
                }
                
                if (loadConfigButton.contains(mousePos)) {
                    availableConfigs = BatchOptimizer::loadResultsFromCSV(configFile);
                    if (!availableConfigs.empty()) {
                        currentConfigIndex = 0;
                        rebuildConfiguration(availableConfigs, currentConfigIndex, scene, primaryCenterX,
                                           sliderSecondaryX, sliderSecondaryY);
                    }
                }

                // Center secondary button
                if (centerSecondaryButton.contains(mousePos)) {
                    HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
                    if (secondaryMirror) {
                        // Center in screen coordinates: middle of window horizontally
                        float screenCenterX = 900.0f;  // Half of 1800
                        float worldCenterX = (screenCenterX - scene.offset.x) / scene.scale;
                        
                        sliderSecondaryX.currentVal = worldCenterX;
                        sliderSecondaryX.updateHandlePosition();
                    }
                }

                // X Increment buttons - coarse
                if (secXDecCoarse.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::max(sliderSecondaryX.minVal, 
                                                           sliderSecondaryX.currentVal - secXDecCoarse.incrementValue);
                    sliderSecondaryX.updateHandlePosition();
                }
                if (secXIncCoarse.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::min(sliderSecondaryX.maxVal, 
                                                           sliderSecondaryX.currentVal + secXIncCoarse.incrementValue);
                    sliderSecondaryX.updateHandlePosition();
                }
                // X Increment buttons - fine
                if (secXDecFine.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::max(sliderSecondaryX.minVal, 
                                                           sliderSecondaryX.currentVal - secXDecFine.incrementValue);
                    sliderSecondaryX.updateHandlePosition();
                }
                if (secXIncFine.contains(mousePos)) {
                    sliderSecondaryX.currentVal = std::min(sliderSecondaryX.maxVal, 
                                                           sliderSecondaryX.currentVal + secXIncFine.incrementValue);
                    sliderSecondaryX.updateHandlePosition();
                }
                
                // Y Increment buttons - coarse
                if (secYDecCoarse.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::max(sliderSecondaryY.minVal, 
                                                           sliderSecondaryY.currentVal - secYDecCoarse.incrementValue);
                    sliderSecondaryY.updateHandlePosition();
                }
                if (secYIncCoarse.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::min(sliderSecondaryY.maxVal, 
                                                           sliderSecondaryY.currentVal + secYIncCoarse.incrementValue);
                    sliderSecondaryY.updateHandlePosition();
                }
                // Y Increment buttons - fine
                if (secYDecFine.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::max(sliderSecondaryY.minVal, 
                                                           sliderSecondaryY.currentVal - secYDecFine.incrementValue);
                    sliderSecondaryY.updateHandlePosition();
                }
                if (secYIncFine.contains(mousePos)) {
                    sliderSecondaryY.currentVal = std::min(sliderSecondaryY.maxVal, 
                                                           sliderSecondaryY.currentVal + secYIncFine.incrementValue);
                    sliderSecondaryY.updateHandlePosition();
                }

                // Optimization buttons
                if (optimizeButton.contains(mousePos) && !isOptimizing) {
                    optimizeButton.setPressed(true);
                    isOptimizing = true;
                    
                    lastOptResult = TelescopeOptimizer::optimizeSecondaryPosition(
                        scene.mirrors, scene.camera, NUM_RAYS,
                        -50.0f, -120.0f, 120.0f,
                        50.0f, 450.0f, 0.5f,
                        sliderSecondaryY.getValue(),
                        sliderSecondaryY.getValue(), 1.0f
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
                    
                    lastOptResult = TelescopeOptimizer::fineOptimize(
                        scene.mirrors, scene.camera, NUM_RAYS,
                        -50.0f, -120.0f, 120.0f,
                        sliderSecondaryX.getValue(),
                        sliderSecondaryY.getValue(),
                        3.0f, 0.1f, 2500
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
            }
            if (event.type == sf::Event::MouseMoved) {
                sliderSecondaryX.handleMouseMove(mousePos);
                sliderSecondaryY.handleMouseMove(mousePos);
                
                secXDecCoarse.setHighlight(secXDecCoarse.contains(mousePos));
                secXIncCoarse.setHighlight(secXIncCoarse.contains(mousePos));
                secXDecFine.setHighlight(secXDecFine.contains(mousePos));
                secXIncFine.setHighlight(secXIncFine.contains(mousePos));
                
                secYDecCoarse.setHighlight(secYDecCoarse.contains(mousePos));
                secYIncCoarse.setHighlight(secYIncCoarse.contains(mousePos));
                secYDecFine.setHighlight(secYDecFine.contains(mousePos));
                secYIncFine.setHighlight(secYIncFine.contains(mousePos));
            }
        }

        window.clear(sf::Color(20, 20, 30));

        HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
        if (secondaryMirror) {
            secondaryMirror->centerX = sliderSecondaryX.getValue();
            secondaryMirror->centerY = sliderSecondaryY.getValue();
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
        
        secXDecCoarse.draw(window);
        secXIncCoarse.draw(window);
        secXDecFine.draw(window);
        secXIncFine.draw(window);
        
        secYDecCoarse.draw(window);
        secYIncCoarse.draw(window);
        secYDecFine.draw(window);
        secYIncFine.draw(window);
        
        prevConfigButton.draw(window);
        nextConfigButton.draw(window);
        loadConfigButton.draw(window);
        centerSecondaryButton.draw(window);
        
        optimizeButton.draw(window);
        fineOptimizeButton.draw(window);

        sf::Text title("Cassegrain Telescope - Config Selector", font, 17);
        title.setFillColor(sf::Color::White);
        title.setPosition(20, 20);
        window.draw(title);

        // Display current configuration info
        std::stringstream configInfo;
        configInfo << "Config " << (currentConfigIndex + 1) << "/" << availableConfigs.size() << ": "
                   << ConfigBuilder::getConfigSummary(availableConfigs[currentConfigIndex]);
        sf::Text configText(configInfo.str(), font, 12);
        configText.setFillColor(sf::Color(150, 200, 255));
        configText.setPosition(20, 50);
        window.draw(configText);

        if (scene.camera) {
            std::stringstream ss;
            ss << "Hits: " << scene.camera->hitPoints.size() << "/" << scene.camera->totalRaysTraced;
            float percentage = scene.camera->totalRaysTraced > 0 ? 
                (100.0f * scene.camera->hitPoints.size()) / scene.camera->totalRaysTraced : 0.0f;
            ss << " (" << std::fixed << std::setprecision(1) << percentage << "%)";
            if (scene.camera->blockedRays > 0) ss << " | Blocked: " << scene.camera->blockedRays;
            
            sf::Text stats(ss.str(), font, 14);
            stats.setFillColor(sf::Color::Cyan);
            stats.setPosition(20, 75);
            window.draw(stats);
            
            if (scene.camera->hitPoints.size() >= 2) {
                std::stringstream focusSS;
                focusSS << "RMS: " << std::fixed << std::setprecision(3) 
                       << scene.camera->getRMSSpotSize() << "mm | Spread: "
                       << scene.camera->getFocusSpread() << "mm";
                
                sf::Text focusStats(focusSS.str(), font, 13);
                focusStats.setFillColor(sf::Color(100, 255, 150));
                focusStats.setPosition(20, 95);
                window.draw(focusStats);
            }
            
            std::stringstream opticalSS;
            float effectiveFocalLength = availableConfigs[currentConfigIndex].systemFocalLength;
            float angularResArcsec = scene.camera->getAngularResolutionArcsec(effectiveFocalLength);
            float fovArcmin = scene.camera->getFieldOfViewArcmin(effectiveFocalLength);
            
            opticalSS << "f_eff:" << std::fixed << std::setprecision(0) << effectiveFocalLength 
                     << "mm | " << std::setprecision(2) << angularResArcsec << "\"/px | FOV:" 
                     << std::setprecision(1) << fovArcmin << "×" 
                     << (fovArcmin * scene.camera->SENSOR_HEIGHT_MM / scene.camera->SENSOR_WIDTH_MM) << "'";
            
            sf::Text opticalSpec(opticalSS.str(), font, 12);
            opticalSpec.setFillColor(sf::Color(200, 200, 255));
            opticalSpec.setPosition(20, 115);
            window.draw(opticalSpec);
        }

        if (lastOptResult.maxHits > 0) {
            std::stringstream ss;
            ss << "Last Opt: X=" << std::fixed << std::setprecision(2) << lastOptResult.bestSecondaryX
               << " Y=" << lastOptResult.bestSecondaryY << " | " << lastOptResult.maxHits << " hits ("
               << std::setprecision(1) << lastOptResult.hitPercentage << "%) RMS:" 
               << std::setprecision(3) << lastOptResult.focusSpread << "mm";
            
            sf::Text optStats(ss.str(), font, 11);
            optStats.setFillColor(sf::Color(100, 255, 100));
            optStats.setPosition(20, 135);
            window.draw(optStats);
        }

        if (isOptimizing) {
            sf::Text optimizingText("Optimizing...", font, 16);
            optimizingText.setFillColor(sf::Color::Yellow);
            optimizingText.setPosition(1200, 850);
            window.draw(optimizingText);
        }

        window.display();
    }
    
    return 0;
}