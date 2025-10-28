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

const int NUM_RAYS = 50;

class Button {
public:
    sf::RectangleShape shape;
    sf::Text label;
    bool isPressed;
    sf::Vector2f basePosition;
    sf::Vector2f baseSize;

    Button(float x, float y, float w, float h, const std::string& text, sf::Font& font) {
        basePosition = sf::Vector2f(x, y);
        baseSize = sf::Vector2f(w, h);
        
        shape.setSize(sf::Vector2f(w * 2, h * 2));
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(50, 120, 200));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);

        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(28);
        label.setFillColor(sf::Color::White);
        
        sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition(
            x + (w * 2 - textBounds.width) / 2.0f,
            y + (h * 2 - textBounds.height) / 2.0f - 4
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
    sf::Vector2f basePosition;
    float baseRadius;
    
    IncrementButton(float x, float y, float radius, const std::string& text, sf::Font& font, float incValue = 1.0f) {
        basePosition = sf::Vector2f(x, y);
        baseRadius = radius;
        
        shape.setRadius(radius * 2);
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(70, 140, 220));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);
        isHovered = false;
        incrementValue = incValue;
        
        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(32);
        label.setFillColor(sf::Color::White);
        label.setStyle(sf::Text::Bold);
        
        sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition(
            x + radius * 2 - textBounds.width / 2.0f,
            y + radius * 2 - textBounds.height / 2.0f - 6
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
    sf::Vector2f basePosition;
    float baseWidth;

    Slider(float x, float y, float w, float min, float max, float init, const std::string& text, sf::Font& font, float step = 1.0f) {
        basePosition = sf::Vector2f(x, y);
        baseWidth = w;
        position = sf::Vector2f(x, y);
        width = w * 2;
        minVal = min;
        maxVal = max;
        currentVal = init;
        stepSize = step;
        isDragging = false;

        track.setSize(sf::Vector2f(w * 2, 8));
        track.setPosition(x, y);
        track.setFillColor(sf::Color(100, 100, 100));

        handle.setRadius(16);
        handle.setFillColor(sf::Color(50, 150, 250));
        handle.setOrigin(16, 16);
        updateHandlePosition();

        label.setFont(font);
        label.setString(text);
        label.setCharacterSize(28);
        label.setFillColor(sf::Color::White);
        label.setPosition(x, y - 40);

        valueText.setFont(font);
        valueText.setCharacterSize(24);
        valueText.setFillColor(sf::Color(150, 200, 255));
        updateValueText();
    }

    void updateHandlePosition() {
        float ratio = (currentVal - minVal) / (maxVal - minVal);
        handle.setPosition(position.x + ratio * width, position.y + 4);
        updateValueText();
    }

    void updateValueText() {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(5) << currentVal;
        valueText.setString(ss.str());
        valueText.setPosition(position.x + width + 20, position.y - 16);
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
    sf::Vector2f baseOffset;
    float baseScale;

    Scene(sf::Vector2f off, float sc) : camera(nullptr), offset(off), scale(sc), 
                                        baseOffset(off), baseScale(sc) {}

    void updateScale(float windowWidth, float windowHeight, float baseWidth, float baseHeight) {
        float scaleX = windowWidth / baseWidth;
        float scaleY = windowHeight / baseHeight;
        float uniformScale = std::min(scaleX, scaleY);
        
        scale = baseScale * uniformScale;
        offset = sf::Vector2f(baseOffset.x * scaleX, baseOffset.y * scaleY);
    }

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
                ray.extend(2000.0f);
                break;
            }
        }
        
        if (ray.bounces == maxBounces) {
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
    scene.mirrors.clear();
    
    ConfigBuilder::buildTelescopeFromConfig(availableConfigs[currentConfigIndex],
                                           scene.mirrors, scene.camera, primaryCenterX);
    
    auto newCamera = std::make_unique<CameraSensor>(
        sf::Vector2f(primaryCenterX + 40.0f, 0.0f), 
        11.2f,
        M_PI / 2.0f, "Camera"
    );
    scene.camera = newCamera.get();
    scene.addMirror(std::move(newCamera));
    
    HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
    if (secondaryMirror) {
        sliderSecondaryX.currentVal = secondaryMirror->centerX;
        sliderSecondaryY.currentVal = secondaryMirror->centerY;
        sliderSecondaryX.updateHandlePosition();
        sliderSecondaryY.updateHandlePosition();
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1800, 1000), "Cassegrain Telescope Ray Tracing", sf::Style::Default);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
        if (!font.loadFromFile("C:\\Windows\\Fonts\\Arial.ttf"))
            if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) 
                return -1;

    std::vector<OpticalConfig> availableConfigs;
    std::string configFile = "optimization_results.csv";
    availableConfigs = BatchOptimizer::loadResultsFromCSV(configFile);
    
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
    float primaryCenterX = 1400.0f;
    
    sf::Vector2u windowSize = window.getSize();
    float uiScaleX = 1.0f;
    float uiScaleY = 1.0f;

    const float BASE_WIDTH = 1800.0f;
    const float BASE_HEIGHT = 1000.0f;

    Slider sliderSecondaryX(50, 850, 280, -2000, 2000, 250, "Secondary X (mm)", font, 0.001f);
    Slider sliderSecondaryY(50, 930, 280, -20, 20, 0, "Secondary Y (mm)", font, 0.001f);
    
    Button prevConfigButton(450, 850, 80, 30, "< Prev", font);
    Button nextConfigButton(450, 920, 80, 30, "Next >", font);
    
    Button loadConfigButton(560, 850, 100, 30, "Load CSV", font);
    Button centerSecondaryButton(560, 920, 140, 30, "Center Secondary", font);

    IncrementButton secXDecCoarse(50, 818, 12, "-", font, 1.0f);
    IncrementButton secXIncCoarse(75, 818, 12, "+", font, 1.0f);
    IncrementButton secXDecFine(100, 818, 10, "-", font, 0.001f);
    IncrementButton secXIncFine(125, 818, 10, "+", font, 0.001f);
    
    IncrementButton secYDecCoarse(50, 898, 12, "-", font, 0.1f);
    IncrementButton secYIncCoarse(75, 898, 12, "+", font, 0.1f);
    IncrementButton secYDecFine(100, 898, 10, "-", font, 0.001f);
    IncrementButton secYIncFine(125, 898, 10, "+", font, 0.001f);

    Button optimizeButton(1200, 850, 150, 30, "Optimize", font);
    Button fineOptimizeButton(1200, 920, 150, 30, "Fine Tune", font);

    bool isOptimizing = false;
    OptimizationResult lastOptResult;

    Scene scene(sf::Vector2f(100, 500), 0.7f);
    
    rebuildConfiguration(availableConfigs, currentConfigIndex, scene, primaryCenterX, 
                        sliderSecondaryX, sliderSecondaryY);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) 
                window.close();

            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                
                windowSize = window.getSize();
                uiScaleX = windowSize.x / BASE_WIDTH;
                uiScaleY = windowSize.y / BASE_HEIGHT;
                
                scene.updateScale(windowSize.x, windowSize.y, BASE_WIDTH, BASE_HEIGHT);
                
                sliderSecondaryX.position = sf::Vector2f(sliderSecondaryX.basePosition.x * uiScaleX, 
                                                         sliderSecondaryX.basePosition.y * uiScaleY);
                sliderSecondaryX.track.setPosition(sliderSecondaryX.position);
                sliderSecondaryX.label.setPosition(sliderSecondaryX.position.x, sliderSecondaryX.position.y - 40);
                sliderSecondaryX.width = sliderSecondaryX.baseWidth * 2 * uiScaleX;
                sliderSecondaryX.track.setSize(sf::Vector2f(sliderSecondaryX.width, 8));
                sliderSecondaryX.updateHandlePosition();
                
                sliderSecondaryY.position = sf::Vector2f(sliderSecondaryY.basePosition.x * uiScaleX, 
                                                         sliderSecondaryY.basePosition.y * uiScaleY);
                sliderSecondaryY.track.setPosition(sliderSecondaryY.position);
                sliderSecondaryY.label.setPosition(sliderSecondaryY.position.x, sliderSecondaryY.position.y - 40);
                sliderSecondaryY.width = sliderSecondaryY.baseWidth * 2 * uiScaleX;
                sliderSecondaryY.track.setSize(sf::Vector2f(sliderSecondaryY.width, 8));
                sliderSecondaryY.updateHandlePosition();
                
                prevConfigButton.shape.setPosition(prevConfigButton.basePosition.x * uiScaleX, 
                                                   prevConfigButton.basePosition.y * uiScaleY);
                prevConfigButton.shape.setSize(sf::Vector2f(prevConfigButton.baseSize.x * 2 * uiScaleX, 
                                                            prevConfigButton.baseSize.y * 2 * uiScaleY));
                prevConfigButton.label.setPosition(prevConfigButton.shape.getPosition().x + 
                    (prevConfigButton.shape.getSize().x - prevConfigButton.label.getLocalBounds().width) / 2.0f,
                    prevConfigButton.shape.getPosition().y + 
                    (prevConfigButton.shape.getSize().y - prevConfigButton.label.getLocalBounds().height) / 2.0f - 4);
                
                nextConfigButton.shape.setPosition(nextConfigButton.basePosition.x * uiScaleX, 
                                                   nextConfigButton.basePosition.y * uiScaleY);
                nextConfigButton.shape.setSize(sf::Vector2f(nextConfigButton.baseSize.x * 2 * uiScaleX, 
                                                            nextConfigButton.baseSize.y * 2 * uiScaleY));
                nextConfigButton.label.setPosition(nextConfigButton.shape.getPosition().x + 
                    (nextConfigButton.shape.getSize().x - nextConfigButton.label.getLocalBounds().width) / 2.0f,
                    nextConfigButton.shape.getPosition().y + 
                    (nextConfigButton.shape.getSize().y - nextConfigButton.label.getLocalBounds().height) / 2.0f - 4);
                
                loadConfigButton.shape.setPosition(loadConfigButton.basePosition.x * uiScaleX, 
                                                   loadConfigButton.basePosition.y * uiScaleY);
                loadConfigButton.shape.setSize(sf::Vector2f(loadConfigButton.baseSize.x * 2 * uiScaleX, 
                                                            loadConfigButton.baseSize.y * 2 * uiScaleY));
                loadConfigButton.label.setPosition(loadConfigButton.shape.getPosition().x + 
                    (loadConfigButton.shape.getSize().x - loadConfigButton.label.getLocalBounds().width) / 2.0f,
                    loadConfigButton.shape.getPosition().y + 
                    (loadConfigButton.shape.getSize().y - loadConfigButton.label.getLocalBounds().height) / 2.0f - 4);
                
                centerSecondaryButton.shape.setPosition(centerSecondaryButton.basePosition.x * uiScaleX, 
                                                       centerSecondaryButton.basePosition.y * uiScaleY);
                centerSecondaryButton.shape.setSize(sf::Vector2f(centerSecondaryButton.baseSize.x * 2 * uiScaleX, 
                                                                 centerSecondaryButton.baseSize.y * 2 * uiScaleY));
                centerSecondaryButton.label.setPosition(centerSecondaryButton.shape.getPosition().x + 
                    (centerSecondaryButton.shape.getSize().x - centerSecondaryButton.label.getLocalBounds().width) / 2.0f,
                    centerSecondaryButton.shape.getPosition().y + 
                    (centerSecondaryButton.shape.getSize().y - centerSecondaryButton.label.getLocalBounds().height) / 2.0f - 4);
                
                float buttonRadius = 2.0f * uiScaleX;
                
                secXDecCoarse.shape.setPosition(secXDecCoarse.basePosition.x * uiScaleX, 
                                               secXDecCoarse.basePosition.y * uiScaleY);
                secXDecCoarse.shape.setRadius(secXDecCoarse.baseRadius * buttonRadius);
                
                secXIncCoarse.shape.setPosition(secXIncCoarse.basePosition.x * uiScaleX, 
                                               secXIncCoarse.basePosition.y * uiScaleY);
                secXIncCoarse.shape.setRadius(secXIncCoarse.baseRadius * buttonRadius);
                
                secXDecFine.shape.setPosition(secXDecFine.basePosition.x * uiScaleX, 
                                             secXDecFine.basePosition.y * uiScaleY);
                secXDecFine.shape.setRadius(secXDecFine.baseRadius * buttonRadius);
                
                secXIncFine.shape.setPosition(secXIncFine.basePosition.x * uiScaleX, 
                                             secXIncFine.basePosition.y * uiScaleY);
                secXIncFine.shape.setRadius(secXIncFine.baseRadius * buttonRadius);
                
                secYDecCoarse.shape.setPosition(secYDecCoarse.basePosition.x * uiScaleX, 
                                               secYDecCoarse.basePosition.y * uiScaleY);
                secYDecCoarse.shape.setRadius(secYDecCoarse.baseRadius * buttonRadius);
                
                secYIncCoarse.shape.setPosition(secYIncCoarse.basePosition.x * uiScaleX, 
                                               secYIncCoarse.basePosition.y * uiScaleY);
                secYIncCoarse.shape.setRadius(secYIncCoarse.baseRadius * buttonRadius);
                
                secYDecFine.shape.setPosition(secYDecFine.basePosition.x * uiScaleX, 
                                             secYDecFine.basePosition.y * uiScaleY);
                secYDecFine.shape.setRadius(secYDecFine.baseRadius * buttonRadius);
                
                secYIncFine.shape.setPosition(secYIncFine.basePosition.x * uiScaleX, 
                                             secYIncFine.basePosition.y * uiScaleY);
                secYIncFine.shape.setRadius(secYIncFine.baseRadius * buttonRadius);
                
                optimizeButton.shape.setPosition(optimizeButton.basePosition.x * uiScaleX, 
                                                optimizeButton.basePosition.y * uiScaleY);
                optimizeButton.shape.setSize(sf::Vector2f(optimizeButton.baseSize.x * 2 * uiScaleX, 
                                                          optimizeButton.baseSize.y * 2 * uiScaleY));
                optimizeButton.label.setPosition(optimizeButton.shape.getPosition().x + 
                    (optimizeButton.shape.getSize().x - optimizeButton.label.getLocalBounds().width) / 2.0f,
                    optimizeButton.shape.getPosition().y + 
                    (optimizeButton.shape.getSize().y - optimizeButton.label.getLocalBounds().height) / 2.0f - 4);
                
                fineOptimizeButton.shape.setPosition(fineOptimizeButton.basePosition.x * uiScaleX, 
                                                     fineOptimizeButton.basePosition.y * uiScaleY);
                fineOptimizeButton.shape.setSize(sf::Vector2f(fineOptimizeButton.baseSize.x * 2 * uiScaleX, 
                                                              fineOptimizeButton.baseSize.y * 2 * uiScaleY));
                fineOptimizeButton.label.setPosition(fineOptimizeButton.shape.getPosition().x + 
                    (fineOptimizeButton.shape.getSize().x - fineOptimizeButton.label.getLocalBounds().width) / 2.0f,
                    fineOptimizeButton.shape.getPosition().y + 
                    (fineOptimizeButton.shape.getSize().y - fineOptimizeButton.label.getLocalBounds().height) / 2.0f - 4);
            }

            sf::Vector2f mousePos;
            if (event.type == sf::Event::MouseButtonPressed)
                mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            if (event.type == sf::Event::MouseMoved)
                mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

            if (event.type == sf::Event::MouseButtonPressed) {
                sliderSecondaryX.handleMousePress(mousePos);
                sliderSecondaryY.handleMousePress(mousePos);

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

                if (centerSecondaryButton.contains(mousePos)) {
                    HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
                    if (secondaryMirror) {
                        float screenCenterX = 900.0f;
                        float worldCenterX = (screenCenterX - scene.offset.x) / scene.scale;
                        
                        sliderSecondaryX.currentVal = worldCenterX;
                        sliderSecondaryX.updateHandlePosition();
                    }
                }

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

                if (optimizeButton.contains(mousePos) && !isOptimizing) {
                    optimizeButton.setPressed(true);
                    isOptimizing = true;
                    
                    HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
                    float currentSecondaryX = secondaryMirror ? secondaryMirror->centerX : 250.0f;
                    
                    lastOptResult = TelescopeOptimizer::optimizeSecondaryPosition(
                        scene.mirrors, scene.camera, NUM_RAYS,
                        -50.0f, -120.0f, 120.0f,
                        currentSecondaryX - 200.0f, currentSecondaryX + 200.0f, 2.0f,
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
        
        // Get primary mirror radius - subtract small epsilon to ensure all rays hit
        float primaryRadius = (availableConfigs[currentConfigIndex].primaryDiameter / 2.0f) - 0.5f;
        
        for (int i = 0; i < NUM_RAYS; i++) {
            float h = -primaryRadius + i * (2.0f * primaryRadius / (NUM_RAYS - 1));
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

        sf::Text title("Cassegrain Telescope - Config Selector", font, 34);
        title.setFillColor(sf::Color::White);
        title.setPosition(20, 20);
        window.draw(title);

        std::stringstream configInfo;
        configInfo << "Config " << (currentConfigIndex + 1) << "/" << availableConfigs.size() << ": "
                   << ConfigBuilder::getConfigSummary(availableConfigs[currentConfigIndex]);
        sf::Text configText(configInfo.str(), font, 24);
        configText.setFillColor(sf::Color(150, 200, 255));
        configText.setPosition(20, 70);
        window.draw(configText);

        if (scene.camera) {
            std::stringstream ss;
            ss << "Hits: " << scene.camera->hitPoints.size() << "/" << scene.camera->totalRaysTraced;
            float percentage = scene.camera->totalRaysTraced > 0 ? 
                (100.0f * scene.camera->hitPoints.size()) / scene.camera->totalRaysTraced : 0.0f;
            ss << " (" << std::fixed << std::setprecision(1) << percentage << "%)";
            if (scene.camera->blockedRays > 0) ss << " | Blocked: " << scene.camera->blockedRays;
            
            sf::Text stats(ss.str(), font, 28);
            stats.setFillColor(sf::Color::Cyan);
            stats.setPosition(20, 110);
            window.draw(stats);
            
            if (scene.camera->hitPoints.size() >= 2) {
                std::stringstream focusSS;
                focusSS << "RMS: " << std::fixed << std::setprecision(3) 
                       << scene.camera->getRMSSpotSize() << "mm | Spread: "
                       << scene.camera->getFocusSpread() << "mm";
                
                sf::Text focusStats(focusSS.str(), font, 26);
                focusStats.setFillColor(sf::Color(100, 255, 150));
                focusStats.setPosition(20, 150);
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
            
            sf::Text opticalSpec(opticalSS.str(), font, 24);
            opticalSpec.setFillColor(sf::Color(200, 200, 255));
            opticalSpec.setPosition(20, 190);
            window.draw(opticalSpec);
            
            HyperbolicMirror* secondaryMirror = dynamic_cast<HyperbolicMirror*>(scene.mirrors[1].get());
            ParabolicMirror* primaryMirror = dynamic_cast<ParabolicMirror*>(scene.mirrors[0].get());
            if (secondaryMirror && primaryMirror) {
                float primaryToSecondary = std::abs(primaryMirror->centerX - secondaryMirror->centerX);
                float secondaryToSensor = std::abs(scene.camera->center.x - secondaryMirror->centerX);
                
                std::stringstream distSS;
                distSS << "Primary -> Secondary: " << std::fixed << std::setprecision(2) << primaryToSecondary 
                       << "mm | Secondary -> Sensor: " << secondaryToSensor << "mm";
                
                sf::Text distText(distSS.str(), font, 24);
                distText.setFillColor(sf::Color(255, 200, 100));
                distText.setPosition(20, 230);
                window.draw(distText);
            }
        }

        if (lastOptResult.maxHits > 0) {
            std::stringstream ss;
            ss << "Last Opt: X=" << std::fixed << std::setprecision(2) << lastOptResult.bestSecondaryX
               << " Y=" << lastOptResult.bestSecondaryY << " | " << lastOptResult.maxHits << " hits ("
               << std::setprecision(1) << lastOptResult.hitPercentage << "%) RMS:" 
               << std::setprecision(3) << lastOptResult.focusSpread << "mm";
            
            sf::Text optStats(ss.str(), font, 22);
            optStats.setFillColor(sf::Color(100, 255, 100));
            optStats.setPosition(20, 270);
            window.draw(optStats);
        }

        if (isOptimizing) {
            sf::Text optimizingText("Optimizing...", font, 32);
            optimizingText.setFillColor(sf::Color::Yellow);
            optimizingText.setPosition(1200 * uiScaleX, 850 * uiScaleY);
            window.draw(optimizingText);
        }

        window.display();
    }
    
    return 0;
}