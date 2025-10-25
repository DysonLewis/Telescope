#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "Ray.h"
#include "Mirror.h"
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <functional>

struct OptimizationResult {
    float bestSecondaryX;
    float bestSecondaryY;
    int maxHits;
    float hitPercentage;
    std::vector<std::pair<float, int>> scanData; // (position, hits) for visualization
};

class TelescopeOptimizer {
public:
    // Optimize secondary mirror position by scanning a range
    static OptimizationResult optimizeSecondaryPosition(
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        float scanXMin,
        float scanXMax,
        float scanXStep,
        float scanYMin = -20.0f,
        float scanYMax = 20.0f,
        float scanYStep = 5.0f,
        int maxBounces = 4
    ) {
        OptimizationResult result;
        result.maxHits = 0;
        result.bestSecondaryX = scanXMin;
        result.bestSecondaryY = 0.0f;

        // Find secondary mirror and camera sensor
        HyperbolicMirror* secondary = nullptr;
        CameraSensor* camera = nullptr;
        
        for (auto& mirror : mirrors) {
            if (mirror->getType() == "hyperbolic") {
                secondary = dynamic_cast<HyperbolicMirror*>(mirror.get());
            }
            if (mirror->getType() == "camera") {
                camera = dynamic_cast<CameraSensor*>(mirror.get());
            }
        }

        if (!secondary || !camera) {
            return result; // Cannot optimize without secondary and camera
        }

        // Store original position
        float originalX = secondary->centerX;
        float originalY = secondary->centerY;

        // Scan through X positions
        for (float x = scanXMin; x <= scanXMax; x += scanXStep) {
            // For each X, scan through Y positions
            for (float y = scanYMin; y <= scanYMax; y += scanYStep) {
                secondary->centerX = x;
                secondary->centerY = y;
                camera->clearHits();

                // Trace rays
                int hits = 0;
                for (int i = 0; i < numRays; i++) {
                    float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
                    Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
                    
                    // Trace ray through telescope
                    traceRay(ray, mirrors, camera, maxBounces);
                }

                hits = camera->hitPoints.size();

                // Store scan data for this X position (use Y=0 data for simplicity)
                if (std::abs(y) < 0.01f) {
                    result.scanData.push_back({x, hits});
                }

                // Update best result
                if (hits > result.maxHits) {
                    result.maxHits = hits;
                    result.bestSecondaryX = x;
                    result.bestSecondaryY = y;
                }
            }
        }

        // Restore original position
        secondary->centerX = originalX;
        secondary->centerY = originalY;

        result.hitPercentage = (100.0f * result.maxHits) / numRays;
        return result;
    }

    // Fine optimization using gradient descent around a starting point
    static OptimizationResult fineOptimize(
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        float startX,
        float startY,
        float searchRadius = 20.0f,
        float initialStep = 5.0f,
        int maxIterations = 20,
        int maxBounces = 4
    ) {
        OptimizationResult result;
        
        HyperbolicMirror* secondary = nullptr;
        CameraSensor* camera = nullptr;
        
        for (auto& mirror : mirrors) {
            if (mirror->getType() == "hyperbolic") {
                secondary = dynamic_cast<HyperbolicMirror*>(mirror.get());
            }
            if (mirror->getType() == "camera") {
                camera = dynamic_cast<CameraSensor*>(mirror.get());
            }
        }

        if (!secondary || !camera) {
            return result;
        }

        float bestX = startX;
        float bestY = startY;
        int bestHits = 0;
        float stepSize = initialStep;

        // Hill climbing optimization
        for (int iter = 0; iter < maxIterations; iter++) {
            bool improved = false;

            // Test 8 directions around current position
            float directions[8][2] = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1},
                {0.707f, 0.707f}, {-0.707f, 0.707f}, {0.707f, -0.707f}, {-0.707f, -0.707f}
            };

            for (auto& dir : directions) {
                float testX = bestX + dir[0] * stepSize;
                float testY = bestY + dir[1] * stepSize;

                // Test this position
                int hits = evaluatePosition(secondary, camera, mirrors, numRays, 
                                           rayStartX, rayYMin, rayYMax, 
                                           testX, testY, maxBounces);

                if (hits > bestHits) {
                    bestHits = hits;
                    bestX = testX;
                    bestY = testY;
                    improved = true;
                }
            }

            // If no improvement, reduce step size
            if (!improved) {
                stepSize *= 0.5f;
                if (stepSize < 0.1f) break;
            }
        }

        result.maxHits = bestHits;
        result.bestSecondaryX = bestX;
        result.bestSecondaryY = bestY;
        result.hitPercentage = (100.0f * bestHits) / numRays;

        return result;
    }

private:
    static void traceRay(Ray& ray, std::vector<std::unique_ptr<Mirror>>& mirrors, 
                        CameraSensor* camera, int maxBounces) {
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
                
                // Check if incoming ray (bounce 0) hit secondary first - skip this ray
                if (bounce == 0 && hitMirror->getType() == "hyperbolic") {
                    ray.bounces = -1; // Mark as invalid
                    break;
                }
                
                // Otherwise reflect off mirror
                ray.reflect(closest.point, closest.normal);
            } else {
                // No intersection, extend ray and stop
                break;
            }
        }
    }

    static int evaluatePosition(
        HyperbolicMirror* secondary,
        CameraSensor* camera,
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        float testX,
        float testY,
        int maxBounces
    ) {
        // Store original position
        float originalX = secondary->centerX;
        float originalY = secondary->centerY;

        // Set test position
        secondary->centerX = testX;
        secondary->centerY = testY;
        camera->clearHits();

        // Trace rays
        for (int i = 0; i < numRays; i++) {
            float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
            Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
            traceRay(ray, mirrors, camera, maxBounces);
        }

        int hits = camera->hitPoints.size();

        // Restore original position
        secondary->centerX = originalX;
        secondary->centerY = originalY;

        return hits;
    }
};

#endif // OPTIMIZER_H