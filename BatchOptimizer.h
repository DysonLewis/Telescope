#ifndef BATCH_OPTIMIZER_H
#define BATCH_OPTIMIZER_H

#include "Ray.h"
#include "Mirror.h"
#include "Camera.h"
#include <string>
#include <vector>
#include <memory>
#include <cmath>

struct OpticalConfig {
    float primaryDiameter;
    float secondaryDiameter;
    float primaryR;
    float secondaryR;
    float primaryF;
    float secondaryF;
    float primaryK;
    float secondaryK;
    float mirrorSeparation;
    float systemFocalLength;
    int rowIndex;
    
    // Optional: results from optimization
    float bestSecondaryX;
    float bestSecondaryY;
    int cameraHits;
    float hitPercentage;
    float rmsSpotSize;
    float score;
};

struct BatchResult {
    OpticalConfig config;
    int cameraHits;
    float hitPercentage;
    float rmsSpotSize;
    float bestSecondaryX;
    float bestSecondaryY;
    float score;  // Combined metric for ranking
};

class BatchOptimizer {
public:
    // Load optical configurations from CSV file
    static std::vector<OpticalConfig> loadConfigsFromCSV(const std::string& filename);
    
    // Load optimization results CSV (includes best positions)
    static std::vector<OpticalConfig> loadResultsFromCSV(const std::string& filename);
    
    // Evaluate a single optical configuration
    static BatchResult evaluateConfig(
        const OpticalConfig& config,
        CameraSensor* camera,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        int maxBounces = 4
    );
    
    // Batch process all configurations and return sorted results
    static std::vector<BatchResult> optimizeBatch(
        const std::string& csvFilename,
        CameraSensor* camera,
        int numRays,
        float rayStartX,
        float rayYMin,
        float rayYMax,
        int maxBounces = 4,
        int topN = 10  // Return top N results
    );
    
    // Save results to CSV
    static void saveResultsToCSV(
        const std::vector<BatchResult>& results,
        const std::string& outputFilename
    );

private:
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    static float stringToFloat(const std::string& str);
};

#endif // BATCH_OPTIMIZER_H