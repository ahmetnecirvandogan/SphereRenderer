# 🌐 Sphere Renderer - COMP 410/510 Programming Assignment #3

**Explore the world through perfect spheres! This OpenGL application brings shading and texture mapping to life, building upon the foundations of Homework 1.**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT) ![Sphere Renderer Demo](path/to/your/cool_sphere_render.gif) ## ✨ Dive into Illuminated Roundness!

This OpenGL program renders a sphere with advanced shading and texture mapping capabilities, fulfilling the requirements of COMP 410/510 Programming Assignment #3. Leveraging vertex and fragment shaders (OpenGL 3.1+), it allows you to:

* **Experience Dynamic Shading:** Toggle between smooth **Gouraud** and per-pixel **Phong** shading (using the `S` key).
* **Master the Modified-Phong Model:** Observe realistic lighting with a single directional light source.
* **Control Light Components:** Individually turn **specular**, **diffuse**, and **ambient** lighting on or off (using the `O` key).
* **Choose Light Behavior:** Keep the light source fixed or have it move with the sphere (toggle with the `L` key).
* **Material Choices:** Switch between **plastic** and **metallic** material properties to see how light interacts differently (using the `M` key).
* **Navigate the Scene:** Zoom in and out to examine details (using the `Z` and `W` keys).
* **Optimize Rendering:** Benefit from enabled **depth testing** and **culling** of back-facing triangles for efficient rendering.
* **Texture Variety:** Toggle between mapping the **earth.ppm** and **basketball.ppm** textures onto the sphere (using the `I` key).
* **Spherical Parametrization:** Witness seamless texture application using the sphere's natural parametrization.
* **Texture Quality:** Explore optimized texture filtering through `glTexParameter` and **mipmapping**.
* **Multiple Display Modes:** Switch between **Wireframe**, **Shading**, and **Texture** display modes (using the `T` key), with **Shading** as the initial default.
* **Informative Help:** Access a command-line help message explaining all keyboard controls (using the `H` key).

## 🛠️ Getting Started - Prepare for Orbital Rendering!

### Prerequisites

Ensure you have a development environment capable of compiling and running OpenGL 3.1+ applications. You will need:

* A C/C++ compiler (e.g., g++, clang, Visual Studio).
* The necessary OpenGL libraries and headers installed on your system.
* The provided `mat.h` and `InitShaders.cpp` files (as per the assignment requirements).

### Installation

1.  Clone the repository (if you're using version control):
    ```bash
    git clone [https://github.com/yourusername/yourrepo.git](https://github.com/yourusername/yourrepo.git)
    cd yourrepo
    ```
2.  Place the provided `mat.h` and `InitShaders.cpp` files in your project directory.
3.  Also, ensure the `earth.ppm` and `basketball.ppm` files are in a location accessible to your program (you might need to create a `textures` subdirectory or similar).

### Compilation

Use the provided `InitShaders.cpp` and `mat.h` along with your main source files to compile the program. Here's a general example using g++:

```bash
g++ -o sphere_renderer main.cpp InitShaders.cpp -lGL -lGLU -lglut # Adjust linker flags based on your system
