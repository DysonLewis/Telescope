#include "Optimizer.h"
#include <limits>
#include <algorithm>
#include <cmath>

OptimizationResult TelescopeOptimizer::optimizeSecondaryPosition(
    std::vector<std::unique_ptr<Mirror>>& mirrors,
    CameraSensor* camera,
    int numRays,
    float rayStartX,
    float rayYMin,
    float rayYMax,
    float scanXMin,
    float scanXMax,
    float scanXStep,
    float scanYMin,
    float scanYMax,
    float scanYStep,
    int maxBounces
) {
    OptimizationResult result;
    result.maxHits = 0;
    result.bestSecondaryX = scanXMin;
    result.bestSecondaryY = 0.0f;

    HyperbolicMirror* secondary = nullptr;
    
    for (auto& mirror : mirrors) {
        if (mirror->getType() == "hyperbolic") {
            secondary = dynamic_cast<HyperbolicMirror*>(mirror.get());
            break;
        }
    }

    if (!secondary || !camera) {
        return result;
    }

    float originalX = secondary->centerX;
    float originalY = secondary->centerY;

    for (float x = scanXMin; x <= scanXMax; x += scanXStep) {
        for (float y = scanYMin; y <= scanYMax; y += scanYStep) {
            secondary->centerX = x;
            secondary->centerY = y;
            camera->clearHits();

            int hits = 0;
            for (int i = 0; i < numRays; i++) {
                float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
                Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
                traceRay(ray, mirrors, camera, maxBounces);
            }

            hits = camera->hitPoints.size();

            if (std::abs(y) < 0.01f) {
                result.scanData.push_back({x, hits});
            }

            if (hits > result.maxHits) {
                result.maxHits = hits;
                result.bestSecondaryX = x;
                result.bestSecondaryY = y;
            }
        }
    }

    secondary->centerX = originalX;
    secondary->centerY = originalY;

    result.hitPercentage = (100.0f * result.maxHits) / numRays;
    
    secondary->centerX = result.bestSecondaryX;
    secondary->centerY = result.bestSecondaryY;
    camera->clearHits();
    
    for (int i = 0; i < numRays; i++) {
        float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
        Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
        traceRay(ray, mirrors, camera, maxBounces);
    }
    
    result.focusSpread = camera->getRMSSpotSize();
    
    secondary->centerX = originalX;
    secondary->centerY = originalY;
    
    return result;
}

OptimizationResult TelescopeOptimizer::fineOptimize(
    std::vector<std::unique_ptr<Mirror>>& mirrors,
    CameraSensor* camera,
    int numRays,
    float rayStartX,
    float rayYMin,
    float rayYMax,
    float startX,
    float startY,
    float searchRadius,
    float initialStep,
    int maxIterations,
    int maxBounces
) {
    OptimizationResult result;
    
    HyperbolicMirror* secondary = nullptr;
    
    for (auto& mirror : mirrors) {
        if (mirror->getType() == "hyperbolic") {
            secondary = dynamic_cast<HyperbolicMirror*>(mirror.get());
            break;
        }
    }

    if (!secondary || !camera) {
        return result;
    }

    float bestX = startX;
    float bestY = startY;
    int bestHits = 0;
    float bestRMS = 1000000.0f;
    float stepSize = initialStep;

    for (int iter = 0; iter < maxIterations; iter++) {
        bool improved = false;

        float directions[8][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {0.707f, 0.707f}, {-0.707f, 0.707f}, {0.707f, -0.707f}, {-0.707f, -0.707f}
        };

        for (auto& dir : directions) {
            float testX = bestX + dir[0] * stepSize;
            float testY = bestY;

            int hits = evaluatePosition(secondary, camera, mirrors, numRays, 
                                       rayStartX, rayYMin, rayYMax, 
                                       testX, testY, maxBounces);

            if (hits > bestHits) {
                bestHits = hits;
                bestX = testX;
                bestY = testY;
            }

            float currentRMS = camera->getRMSSpotSize();
            if (currentRMS < bestRMS) {
                bestRMS = currentRMS;
                bestX = testX;
                bestY = testY;
                improved = true;
            }
        }

        if (!improved) {
            stepSize *= 0.5f;
            if (stepSize < 0.001f) break;
        }
    }

    result.maxHits = bestHits;
    result.bestSecondaryX = bestX;
    result.bestSecondaryY = bestY;
    result.hitPercentage = (100.0f * bestHits) / numRays;

    secondary->centerX = bestX;
    secondary->centerY = bestY;
    camera->clearHits();
    
    for (int i = 0; i < numRays; i++) {
        float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
        Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
        traceRay(ray, mirrors, camera, maxBounces);
    }
    
    result.focusSpread = camera->getRMSSpotSize();

    return result;
}

void TelescopeOptimizer::traceRay(Ray& ray, std::vector<std::unique_ptr<Mirror>>& mirrors, 
                                  CameraSensor* camera, int maxBounces) {
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
            break;
        }
    }
    
    if (ray.bounces >= 0 && camera) {
        camera->totalRaysTraced++;
    }
}

int TelescopeOptimizer::evaluatePosition(
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
    float originalX = secondary->centerX;
    float originalY = secondary->centerY;

    secondary->centerX = testX;
    secondary->centerY = testY;
    camera->clearHits();

    for (int i = 0; i < numRays; i++) {
        float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
        Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
        traceRay(ray, mirrors, camera, maxBounces);
    }

    int hits = camera->hitPoints.size();

    secondary->centerX = originalX;
    secondary->centerY = originalY;

    return hits;
}