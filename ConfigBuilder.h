#ifndef CONFIG_BUILDER_H
#define CONFIG_BUILDER_H

#include "Mirror.h"
#include "Camera.h"
#include "BatchOptimizer.h"
#include <memory>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

// Helper class to build telescope configuration from OpticalConfig
class ConfigBuilder {
public:
    static void buildTelescopeFromConfig(
        const OpticalConfig& config,
        std::vector<std::unique_ptr<Mirror>>& mirrors,
        CameraSensor* camera,
        float primaryCenterX  // Note: This is pass-by-value, not reference
    ) {
        // Clear existing mirrors
        mirrors.clear();
        
        // Calculate primary mirror parameters
        float primaryYMax = config.primaryDiameter / 2.0f;
        float primaryYMin = -primaryYMax;
        float holeRadius = config.secondaryDiameter / 2.0f;
        
        // Create primary parabolic mirror
        auto primary = std::make_unique<ParabolicMirror>(
            config.primaryF,
            primaryYMin,
            primaryYMax,
            primaryCenterX,
            "Primary",
            holeRadius
        );
        
        // Calculate secondary mirror parameters
        float secondaryA = std::abs(config.secondaryR) / 2.0f;
        float secondaryB = secondaryA * std::sqrt(std::abs(config.secondaryK + 1.0f));
        float secondaryYMax = config.secondaryDiameter / 2.0f;
        float secondaryYMin = -secondaryYMax;
        
        // Calculate secondary position
        float secondaryX = config.bestSecondaryX > 0.0f ? 
            config.bestSecondaryX : 
            (primaryCenterX - config.primaryF + config.mirrorSeparation);
        float secondaryY = config.bestSecondaryY;
        
        // Create secondary hyperbolic mirror
        auto secondary = std::make_unique<HyperbolicMirror>(
            secondaryX,
            secondaryY,
            secondaryA,
            secondaryB,
            secondaryYMin,
            secondaryYMax,
            true,  // Left branch (convex)
            "Secondary"
        );
        
        mirrors.push_back(std::move(primary));
        mirrors.push_back(std::move(secondary));
        
        // Note: Camera is updated separately in main
    }
    
    static std::string getConfigSummary(const OpticalConfig& config) {
        std::stringstream ss;
        ss << "Primary: " << std::fixed << std::setprecision(1) 
           << config.primaryDiameter << "mm, f=" << config.primaryF << "mm | "
           << "Secondary: " << config.secondaryDiameter << "mm, k=" 
           << std::setprecision(2) << config.secondaryK << " | "
           << "System f=" << std::setprecision(0) << config.systemFocalLength << "mm";
        return ss.str();
    }
};

#endif // CONFIG_BUILDER_H