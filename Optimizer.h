#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "Ray.h"
#include "Mirror.h"
#include "Camera.h"
#include <vector>
#include <memory>
#include <utility>

struct OptimizationResult {
    float bestSecondaryX;
    float bestSecondaryY;
    int maxHits;
    float hitPercentage;
    float focusSpread;
    std::vector<std::pair<float, int>> scanData;
};

class TelescopeOptimizer {
public:
    static OptimizationResult optimizeSecondaryPosition(
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        CameraSensor* camera,
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
    );

    static OptimizationResult fineOptimize(
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        CameraSensor* camera,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        float startX,
        float startY,
        float searchRadius = 20.0f,
        float initialStep = 0.5f,
        int maxIterations = 10000000,
        int maxBounces = 4
    );

private:
    static void traceRay(Ray& ray, std::vector<std::unique_ptr<Mirror>>& mirrors, 
                        CameraSensor* camera, int maxBounces);

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
    );
};

#endif // OPTIMIZER_H