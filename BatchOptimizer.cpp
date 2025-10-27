#include "BatchOptimizer.h"
#include "Optimizer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

std::vector<std::string> BatchOptimizer::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

float BatchOptimizer::stringToFloat(const std::string& str) {
    try {
        return std::stof(str);
    } catch (...) {
        return 0.0f;
    }
}

std::vector<OpticalConfig> BatchOptimizer::loadConfigsFromCSV(const std::string& filename) {
    std::vector<OpticalConfig> configs;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return configs;
    }
    
    std::string line;
    bool firstLine = true;
    int rowIndex = 0;
    
    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue;  // Skip header
        }
        
        std::vector<std::string> tokens = splitString(line, ',');
        if (tokens.size() >= 10) {
            OpticalConfig config;
            config.primaryDiameter = stringToFloat(tokens[0]);
            config.secondaryDiameter = stringToFloat(tokens[1]);
            config.primaryR = stringToFloat(tokens[2]);
            config.secondaryR = stringToFloat(tokens[3]);
            config.primaryF = stringToFloat(tokens[4]);
            config.secondaryF = stringToFloat(tokens[5]);
            config.primaryK = stringToFloat(tokens[6]);
            config.secondaryK = stringToFloat(tokens[7]);
            config.mirrorSeparation = stringToFloat(tokens[8]);
            config.systemFocalLength = stringToFloat(tokens[9]);
            config.rowIndex = rowIndex++;
            
            // Initialize optional fields
            config.bestSecondaryX = 0.0f;
            config.bestSecondaryY = 0.0f;
            config.cameraHits = 0;
            config.hitPercentage = 0.0f;
            config.rmsSpotSize = 0.0f;
            config.score = 0.0f;
            
            configs.push_back(config);
        }
    }
    
    file.close();
    std::cout << "Loaded " << configs.size() << " optical configurations from " << filename << std::endl;
    return configs;
}

std::vector<OpticalConfig> BatchOptimizer::loadResultsFromCSV(const std::string& filename) {
    std::vector<OpticalConfig> configs;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return configs;
    }
    
    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue;  // Skip header
        }
        
        std::vector<std::string> tokens = splitString(line, ',');
        // Rank,Score,CameraHits,HitPercentage,RMSSpotSize,BestSecondaryX,BestSecondaryY,
        // PrimaryDiameter,SecondaryDiameter,PrimaryR,SecondaryR,PrimaryF,SecondaryF,
        // PrimaryK,SecondaryK,MirrorSeparation,SystemFocalLength,OriginalRowIndex
        if (tokens.size() >= 18) {
            OpticalConfig config;
            // Skip rank (tokens[0])
            config.score = stringToFloat(tokens[1]);
            config.cameraHits = static_cast<int>(stringToFloat(tokens[2]));
            config.hitPercentage = stringToFloat(tokens[3]);
            config.rmsSpotSize = stringToFloat(tokens[4]);
            config.bestSecondaryX = stringToFloat(tokens[5]);
            config.bestSecondaryY = stringToFloat(tokens[6]);
            config.primaryDiameter = stringToFloat(tokens[7]);
            config.secondaryDiameter = stringToFloat(tokens[8]);
            config.primaryR = stringToFloat(tokens[9]);
            config.secondaryR = stringToFloat(tokens[10]);
            config.primaryF = stringToFloat(tokens[11]);
            config.secondaryF = stringToFloat(tokens[12]);
            config.primaryK = stringToFloat(tokens[13]);
            config.secondaryK = stringToFloat(tokens[14]);
            config.mirrorSeparation = stringToFloat(tokens[15]);
            config.systemFocalLength = stringToFloat(tokens[16]);
            config.rowIndex = static_cast<int>(stringToFloat(tokens[17]));
            
            configs.push_back(config);
        }
    }
    
    file.close();
    std::cout << "Loaded " << configs.size() << " optimized configurations from " << filename << std::endl;
    return configs;
}

BatchResult BatchOptimizer::evaluateConfig(
    const OpticalConfig& config,
    CameraSensor* camera,
    int numRays,
    float rayStartX,
    float rayYMin,
    float rayYMax,
    int maxBounces
) {
    BatchResult result;
    result.config = config;
    result.cameraHits = 0;
    result.hitPercentage = 0.0f;
    result.rmsSpotSize = 0.0f;
    result.bestSecondaryX = 0.0f;
    result.bestSecondaryY = 0.0f;
    result.score = 0.0f;
    
    // Create mirrors based on configuration
    std::vector<std::unique_ptr<Mirror>> mirrors;
    
    // Primary parabolic mirror
    float primaryYMax = config.primaryDiameter / 2.0f;
    float primaryYMin = -primaryYMax;
    float primaryCenterX = 500.0f;  // Fixed position
    float holeRadius = config.secondaryDiameter / 2.0f + 5.0f;  // Slightly larger than secondary
    
    auto primary = std::make_unique<ParabolicMirror>(
        config.primaryF,
        primaryYMin,
        primaryYMax,
        primaryCenterX,
        "Primary",
        holeRadius
    );
    
    // Secondary hyperbolic mirror
    // Calculate semi-axes from R and k
    float secondaryA = std::abs(config.secondaryR) / 2.0f;
    float secondaryB = secondaryA * std::sqrt(std::abs(config.secondaryK + 1.0f));
    
    float secondaryYMax = config.secondaryDiameter / 2.0f;
    float secondaryYMin = -secondaryYMax;
    
    // Initial position: in front of primary focal point
    float initialSecondaryX = primaryCenterX - config.primaryF + config.mirrorSeparation;
    
    auto secondary = std::make_unique<HyperbolicMirror>(
        initialSecondaryX,
        0.0f,
        secondaryA,
        secondaryB,
        secondaryYMin,
        secondaryYMax,
        true,  // Left branch (convex)
        "Secondary"
    );
    
    mirrors.push_back(std::move(primary));
    mirrors.push_back(std::move(secondary));
    
    // Run quick optimization to find best secondary position
    HyperbolicMirror* secondaryPtr = dynamic_cast<HyperbolicMirror*>(mirrors[1].get());
    
    if (!secondaryPtr || !camera) {
        return result;
    }
    
    // Scan range around initial position
    float scanXMin = initialSecondaryX - 50.0f;
    float scanXMax = initialSecondaryX + 50.0f;
    float scanXStep = 2.0f;
    
    int bestHits = 0;
    float bestX = initialSecondaryX;
    float bestY = 0.0f;
    float bestRMS = 100000.0f;
    
    for (float x = scanXMin; x <= scanXMax; x += scanXStep) {
        secondaryPtr->centerX = x;
        secondaryPtr->centerY = 0.0f;
        camera->clearHits();
        
        // Trace rays
        for (int i = 0; i < numRays; i++) {
            float h = rayYMin + i * (rayYMax - rayYMin) / (numRays - 1);
            Ray ray(sf::Vector2f(rayStartX, h), sf::Vector2f(1.0f, 0.0f));
            
            // Trace through system
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
                        camera->hitPoints.push_back(closest.point);
                        break;
                    }
                    
                    if (bounce == 0 && hitMirror->getType() == "hyperbolic") {
                        ray.bounces = -1;
                        if (camera) camera->blockedRays++;
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
        
        int hits = camera->hitPoints.size();
        float rms = camera->getRMSSpotSize();
        
        // Prefer configurations with more hits, then smaller RMS
        if (hits > bestHits || (hits == bestHits && rms < bestRMS)) {
            bestHits = hits;
            bestX = x;
            bestY = 0.0f;
            bestRMS = rms;
        }
    }
    
    result.cameraHits = bestHits;
    result.hitPercentage = (100.0f * bestHits) / numRays;
    result.rmsSpotSize = bestRMS;
    result.bestSecondaryX = bestX;
    result.bestSecondaryY = bestY;
    
    // Calculate combined score (higher is better)
    // Prioritize hit percentage, then minimize RMS spot size
    result.score = result.hitPercentage * 100.0f - result.rmsSpotSize;
    
    return result;
}

std::vector<BatchResult> BatchOptimizer::optimizeBatch(
    const std::string& csvFilename,
    CameraSensor* camera,
    int numRays,
    float rayStartX,
    float rayYMin,
    float rayYMax,
    int maxBounces,
    int topN
) {
    std::vector<OpticalConfig> configs = loadConfigsFromCSV(csvFilename);
    std::vector<BatchResult> results;
    
    int totalConfigs = configs.size();
    int processedCount = 0;
    
    std::cout << "Evaluating " << totalConfigs << " configurations..." << std::endl;
    
    for (const auto& config : configs) {
        BatchResult result = evaluateConfig(
            config, camera, numRays,
            rayStartX, rayYMin, rayYMax, maxBounces
        );
        results.push_back(result);
        
        processedCount++;
        if (processedCount % 100 == 0 || processedCount == totalConfigs) {
            std::cout << "Progress: " << processedCount << "/" << totalConfigs 
                     << " (" << (100 * processedCount / totalConfigs) << "%)" << std::endl;
        }
    }
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end(),
              [](const BatchResult& a, const BatchResult& b) {
                  return a.score > b.score;
              });
    
    // Return top N
    if (results.size() > static_cast<size_t>(topN)) {
        results.resize(topN);
    }
    
    std::cout << "\nTop " << results.size() << " configurations found!" << std::endl;
    return results;
}

void BatchOptimizer::saveResultsToCSV(
    const std::vector<BatchResult>& results,
    const std::string& outputFilename
) {
    std::ofstream file(outputFilename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not create output file " << outputFilename << std::endl;
        return;
    }
    
    // Write header
    file << "Rank,Score,CameraHits,HitPercentage,RMSSpotSize,BestSecondaryX,BestSecondaryY,"
         << "PrimaryDiameter,SecondaryDiameter,PrimaryR,SecondaryR,PrimaryF,SecondaryF,"
         << "PrimaryK,SecondaryK,MirrorSeparation,SystemFocalLength,OriginalRowIndex\n";
    
    // Write results
    int rank = 1;
    for (const auto& result : results) {
        file << rank++ << ","
             << std::fixed << std::setprecision(2) << result.score << ","
             << result.cameraHits << ","
             << result.hitPercentage << ","
             << result.rmsSpotSize << ","
             << result.bestSecondaryX << ","
             << result.bestSecondaryY << ","
             << result.config.primaryDiameter << ","
             << result.config.secondaryDiameter << ","
             << result.config.primaryR << ","
             << result.config.secondaryR << ","
             << result.config.primaryF << ","
             << result.config.secondaryF << ","
             << result.config.primaryK << ","
             << result.config.secondaryK << ","
             << result.config.mirrorSeparation << ","
             << result.config.systemFocalLength << ","
             << result.config.rowIndex << "\n";
    }
    
    file.close();
    std::cout << "Results saved to " << outputFilename << std::endl;
}