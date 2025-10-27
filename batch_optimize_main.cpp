#include "BatchOptimizer.h"
#include "Camera.h"
#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
    std::string inputFile = "cassegrain_optics_grid.csv";
    std::string outputFile = "optimization_results.csv";
    int topN = 20;
    int numRays = 500;  // Reduced for faster batch processing
    
    // Parse command line arguments
    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        outputFile = argv[2];
    }
    if (argc >= 4) {
        topN = std::stoi(argv[3]);
    }
    if (argc >= 5) {
        numRays = std::stoi(argv[4]);
    }
    
    std::cout << "=== Cassegrain Telescope Batch Optimizer ===" << std::endl;
    std::cout << "Input CSV: " << inputFile << std::endl;
    std::cout << "Output CSV: " << outputFile << std::endl;
    std::cout << "Top N configurations: " << topN << std::endl;
    std::cout << "Rays per test: " << numRays << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;
    
    // Create camera sensor with specifications
    CameraSensor camera(
        sf::Vector2f(540.0f, 0.0f),  // Position (will be adjusted per config)
        40.0f,                        // Width
        M_PI / 2.0f,                  // Angle (vertical)
        "Camera"
    );
    
    // Run batch optimization
    std::vector<BatchResult> results = BatchOptimizer::optimizeBatch(
        inputFile,
        &camera,
        numRays,
        -50.0f,   // Ray start X
        -120.0f,  // Ray Y min
        120.0f,   // Ray Y max
        4,        // Max bounces
        topN
    );
    
    // Display top results
    std::cout << "\n=== Top " << results.size() << " Configurations ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    for (size_t i = 0; i < results.size() && i < 10; i++) {
        const auto& r = results[i];
        std::cout << "\nRank #" << (i + 1) << ":" << std::endl;
        std::cout << "  Score: " << r.score << std::endl;
        std::cout << "  Camera Hits: " << r.cameraHits << " (" << r.hitPercentage << "%)" << std::endl;
        std::cout << "  RMS Spot: " << r.rmsSpotSize << " mm" << std::endl;
        std::cout << "  Primary: " << r.config.primaryDiameter << "mm diam, "
                 << "f=" << r.config.primaryF << "mm" << std::endl;
        std::cout << "  Secondary: " << r.config.secondaryDiameter << "mm diam, "
                 << "R=" << r.config.secondaryR << "mm, k=" << r.config.secondaryK << std::endl;
        std::cout << "  Mirror Sep: " << r.config.mirrorSeparation << "mm" << std::endl;
        std::cout << "  System f: " << r.config.systemFocalLength << "mm" << std::endl;
        std::cout << "  Best Secondary Pos: X=" << r.bestSecondaryX 
                 << ", Y=" << r.bestSecondaryY << std::endl;
    }
    
    // Save all results to CSV
    BatchOptimizer::saveResultsToCSV(results, outputFile);
    
    std::cout << "\n=== Optimization Complete ===" << std::endl;
    std::cout << "Full results saved to: " << outputFile << std::endl;
    std::cout << "You can load the best configuration into optic_raytracer for visualization." << std::endl;
    
    return 0;
}
