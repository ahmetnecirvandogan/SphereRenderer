#include "ppm_loader.h" // Include your new header file

#include <iostream>
#include <fstream>
#include <sstream> // Required for std::stringstream

// Function definition for loading a P3 PPM file
PPMImage loadPPM(const std::string& filename) {
    PPMImage image; // Resulting image object
    std::ifstream ifs(filename);

    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open PPM file: " << filename << std::endl;
        image.isValid = false;
        return image;
    }

    std::string magicNumber;
    ifs >> magicNumber; // Read the magic number ("P3")

    if (magicNumber != "P3") {
        std::cerr << "Error: " << filename << " is not a valid P3 PPM file. Magic number was: " << magicNumber << std::endl;
        image.isValid = false;
        ifs.close();
        return image;
    }
    // Optional: Informative message
    // std::cout << filename << " identified as a P3 PPM file." << std::endl;

    // Consume the rest of the line after "P3" if any characters are there
    std::string restOfMagicNumberLine;
    std::getline(ifs, restOfMagicNumberLine);

    // Variables to store dimensions and max color value
    int width = 0, height = 0, maxColorValue = 0;
    int* valuesToRead[] = {&width, &height, &maxColorValue};
    int valuesReadCount = 0;
    std::string currentLine;

    // Read width, height, and maxColorValue, skipping comments
    while (valuesReadCount < 3 && std::getline(ifs, currentLine)) {
        if (currentLine.empty() || currentLine[0] == '#') { // Skip empty lines and comments
            // Optional: print comments
            // if (currentLine[0] == '#') std::cout << "Comment: " << currentLine << std::endl;
            continue;
        }

        std::stringstream ss(currentLine);
        int valueOnLine;
        while (ss >> valueOnLine) { // Read all integer values on the current line
            if (valuesReadCount < 3) {
                *valuesToRead[valuesReadCount] = valueOnLine;
                valuesReadCount++;
            }
        }
    }

    if (valuesReadCount < 3 || width <= 0 || height <= 0 || maxColorValue <= 0) {
        std::cerr << "Error: Invalid or incomplete PPM header (width, height, maxVal) in " << filename << "." << std::endl;
        std::cerr << "Read: width=" << width << ", height=" << height << ", maxVal=" << maxColorValue << std::endl;
        image.isValid = false;
        ifs.close();
        return image;
    }

    image.width = width;
    image.height = height;
    // Optional: Print dimensions
    // std::cout << "Dimensions: " << image.width << "x" << image.height << ", Max Color Value: " << maxColorValue << std::endl;

    // Allocate memory for image data (RGB, so 3 bytes per pixel)
    image.data.resize(static_cast<size_t>(image.width) * image.height * 3);

    int r, g, b; // Temporary variables for reading pixel components
    for (size_t i = 0; i < static_cast<size_t>(image.width) * image.height; ++i) {
        if (!(ifs >> r >> g >> b)) { // Read RGB values for each pixel
            std::cerr << "Error: Failed to read pixel data at pixel index " << i << " from " << filename << "." << std::endl;
            // More detailed stream state for debugging:
            // std::cerr << "Stream state: good=" << ifs.good() << " eof=" << ifs.eof() << " fail=" << ifs.fail() << " bad=" << ifs.bad() << std::endl;
            image.isValid = false;
            image.data.clear(); // Clear partially filled data
            ifs.close();
            return image;
        }
        // Assuming maxColorValue is 255. If it could be different and pixel values need scaling,
        // that logic would go here. For typical 8-bit PPMs, maxColorValue is 255.
        image.data[i * 3 + 0] = static_cast<unsigned char>(r);
        image.data[i * 3 + 1] = static_cast<unsigned char>(g);
        image.data[i * 3 + 2] = static_cast<unsigned char>(b);
    }

    ifs.close();
    image.isValid = true;
    std::cout << "Successfully loaded PPM file: " << filename << std::endl;
    return image;
}
