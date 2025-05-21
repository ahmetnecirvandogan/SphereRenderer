#ifndef PPM_LOADER_H
#define PPM_LOADER_H

#include <string>
#include <vector>

// Structure to hold image data and dimensions
struct PPMImage {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> data; // Stores RGB, RGB, RGB...
    bool isValid = false;            // Flag to indicate if loading was successful
};

// Function declaration for loading a P3 PPM file
PPMImage loadPPM(const std::string& filename);

#endif // PPM_LOADER_H
