# 🌐 Sphere Renderer

## Project Overview

This project extends a basic OpenGL application to incorporate advanced 3D rendering techniques, including lighting, shading, material properties, and texture mapping. The central scene features a sphere object that exhibits physics-based bouncing behavior. The implementation uses modern OpenGL (version 3.3+) with GLSL shaders for per-pixel and per-vertex lighting, and texture application.

## Features Implemented

The application demonstrates a variety of rendering techniques and interactive controls:

* **Core Graphics & Object:**
    * Rendering of a 3D sphere with perspective projection.
    * Physics simulation for the sphere (gravity, bouncing, air resistance).
* **Lighting & Shading:**
    * Single directional light source.
    * Switchable shading models: Gouraud (per-vertex) and Phong (per-fragment).
    * Modified Phong illumination model with toggleable ambient, diffuse, and specular components.
    * Light source position can be fixed in world space or move relative to the object.
    * Depth testing and back-face culling enabled.
* **Materials:**
    * Two distinct material types (e.g., "plastic" and "metallic") affecting specular highlights and shininess, toggleable by the user.
* **Texture Mapping:**
    * Custom PPM image loader for P3 format 2D textures (`earth.ppm`, `basketball.ppm`).
    * Spherical texture mapping for 2D textures with parametrically generated coordinates.
    * **Bonus Feature: 1D Texture Mapping:**
        * A synthetic 1D striped texture is procedurally generated.
        * This 1D texture is mapped to the sphere based on vertex distance from a defined world-space plane.
* **Display Modes:**
    * User-selectable display modes:
        * Shading (Phong/Gouraud)
        * Shading with planar projection shadow (Bonus Feature)
        * Wireframe
        * Texture Mapping (combines lighting with 1D or 2D textures)
* **User Interface & Controls:**
    * Interactive keyboard controls for toggling features, display modes, camera zoom, and object reset.
    * On-screen help (printed to console) detailing all controls.

## Requirements & Dependencies

* C++ Compiler (supporting C++11 or later)
* OpenGL 3.3+ capable graphics card and drivers
* GLEW (The OpenGL Extension Wrangler Library)
* GLFW (for windowing and input)
* Provided "Angel" utility code (e.g., `mat.h` for vector/matrix operations, `InitShader.cpp` for shader loading)

## Compilation and Execution (General Guidance)

1.  **Ensure Dependencies are Met:** Make sure GLEW and GLFW libraries and headers are installed and accessible to your compiler/linker. The Angel utilities (`mat.h`, `vec.h`, `CheckError.h`, `InitShader.cpp`) should be part of the project structure.
2.  **Place Texture Files:** Ensure `earth.ppm` and `basketball.ppm` are in the correct runtime directory accessible by the executable.
3.  **Compile:** Use a C++ compiler (like g++ or Clang) to compile all `.cpp` source files (`main.cpp`, `PhysicsObject.cpp`, `Material.cpp`, `ppm_loader.cpp`, `Light.cpp`, `InitShader.cpp`) and link against OpenGL, GLEW, and GLFW libraries.
    * Example (macOS with Homebrew, may vary):
        ```bash
        g++ -std=c++11 -o sphere_renderer src/*.cpp src/Glad/src/glad.c -Iinclude -Isrc/Glad/include -L/usr/local/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        ```
    * (Adjust include paths `-I`, library paths `-L`, and linked libraries `-l` based on your system and how you've installed the dependencies.)
4.  **Run:** Execute the compiled program.
    ```bash
    ./sphere_renderer
    ```

## Keyboard Controls

* **H**: Display help (list of controls) in the console.
* **Q / ESC**: Quit the application.
* **R**: Reset the sphere's position and velocity.
* **S**: Toggle shading mode (Phong / Gouraud).
* **O**: Cycle through toggling Ambient, Diffuse, and Specular light components.
* **L**: Toggle light source position (fixed in world / moves with object).
* **M**: Toggle material properties (Plastic / Metallic).
* **Z**: Zoom In.
* **W**: Zoom Out.
* **T**: Cycle through display modes (Shading, Shading + Shadow, Wireframe, Texture).
* **I**: Cycle through active textures (Earth 2D, Basketball 2D, Synthetic 1D) when in Texture display mode.

## Code Structure

* `main.cpp`: Core application logic, OpenGL setup, rendering loop, event handling.
* `PhysicsObject.cpp/.h`: Defines the sphere's physics and behavior.
* `Material.cpp/.h`: Manages material properties for lighting.
* `ppm_loader.cpp/.h`: Implements loading of PPM image files for 2D textures.
* `light.cpp/.h`: Defines a `Light` class structure (Note: directional light parameters are currently set directly as uniforms in `main.cpp`).
* `vshader.glsl`: Vertex shader responsible for vertex transformations, normal transformations, Gouraud shading, and 1D/2D texture coordinate processing.
* `fshader.glsl`: Fragment shader responsible for Phong shading, texture sampling (1D and 2D), combining textures with lighting, and shadow rendering.
* `include/Angel.h` (and related files): Provided library for vector/matrix math and shader initialization.

## Author

AHMET NEÇİRVAN DOĞAN
andogan22@ku.edu.tr
