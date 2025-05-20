#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Angel.h"
#include "PhysicsObject.h"
#include <vector>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "light.h"
#include "Material.h"
#include <cmath>
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
inline float degreesToRadians(float degrees) {
    return degrees * (float(M_PI) / 180.0f);
}

GLuint program;
GLuint texture; // Texture ID

//GLuint backgroundVAO, backgroundVBO; // VAO and VBO for background quad
double frameRate = 120;
double deltaTime = 1.0 / frameRate;
vec3 initialVelocity;
typedef vec4  point4;
void bindObject(GLuint vPosition);

// About the scene
int sceneWidth = 1200;
int sceneHeight = 600;

// About the object
PhysicsObject bouncingObject;
vec3 computeInitialPosition(float objectSize);
std::vector<vec3> normals_sphere;
std::vector<vec4> colors_sphere;

//drawing mode
GLenum drawingMode = GL_FILL;

// setup for sphere
const int latitudeBands = 50; //Divides the sphere along the vertical axis
const int longitudeBands = 50; //Divides the sphere along the horizontal axis
std::vector<point4> points_sphere;
std::vector<GLuint> indices_sphere; // Add indices vector
GLuint sphereVAO, sphereVBO, sphereIBO; // IBO for sphere

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;
// New Camera Parameters
vec3 gCameraEye = vec3(0.0f, 0.5f, 3.0f); // Initial camera position (looking slightly down and from a distance)
vec3 gCameraAt = vec3(0.0f, 0.0f, 0.0f);  // Point the camera is looking at (center of the scene)
vec3 gCameraUp = vec3(0.0f, 1.0f, 0.0f);  // Up direction for the camera
GLfloat gInitialFOVy = 45.0f; // Initial Field of View in Y direction (in degrees)
GLfloat gZNear = 0.1f;        // Near clipping plane
GLfloat gZFar = 100.0f;       // Far clipping plane

mat4 gProjectionMatrix; // Global projection matrix
float gZoomFactor = 1.0f; // Global zoom factor
const float gZoomStepFactor = 0.1f;

// Color uniform location
GLuint colorLocation;
vec4 currentColor = vec4(1.0, 0.0f, 0.0f, 1.0f); //red

Light lightSource;
GLuint vPosition;

Material plasticMaterial(0.5f, 32.0f);   // Example: moderate specular intensity, moderate shininess
Material metallicMaterial(0.8f, 128.0f); // Example: high specular intensity, high shininess
Material currentMaterial;                // The material currently in use

// Uniform locations for material properties
GLuint materialSpecularIntensityLoc;
GLuint materialShininessLoc;

float backgroundVertices[] = {
    // positions         // texture coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int Axis = Yaxis;
GLfloat Theta[NumAxes] = { 0.0, 0.0, 0.0 };
float rotationSpeed = 50.0f;

// Helper function to update the projection matrix
void updateProjection() {
    if (sceneHeight == 0) sceneHeight = 1; // Prevent division by zero
    float aspect = (float)sceneWidth / (float)sceneHeight;

    GLfloat current_fovy = gInitialFOVy * gZoomFactor;

    // Clamp FOV to reasonable limits
    if (current_fovy < 1.0f) current_fovy = 1.0f;   // Prevent it from becoming too narrow
    if (current_fovy > 120.0f) current_fovy = 120.0f; // Prevent it from becoming too wide

    gProjectionMatrix = Perspective(current_fovy, aspect, gZNear, gZFar);

    // Ensure the correct shader program is active
    glUseProgram(program);
    // Send the projection matrix to the shader
    glUniformMatrix4fv(Projection, 1, GL_TRUE, gProjectionMatrix);
}

//For the Background
void loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (nrChannels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (nrChannels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load texture" << std::endl;
        stbi_image_free(data);
    }
}

/*
void setupBackground() {
    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);

    glBindVertexArray(backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}
*/
// generation of the sphere
void generateSphere(float radius) {
    points_sphere.clear();
    indices_sphere.clear();
    normals_sphere.clear(); // Clear normals
    colors_sphere.clear();  // Clear colors

    for (int lat = 0; lat <= latitudeBands; lat++) {
        float theta = lat * M_PI / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= longitudeBands; lon++) {
            float phi = lon * 2 * M_PI / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            point4 pos = point4(radius * x, radius * y, radius * z, 1.0);
            points_sphere.push_back(pos);

            // Calculate and store normal (normalize the position for a unit sphere, then scale for your sphere)
            vec3 normal = normalize(vec3(x, y, z)); // Assuming sphere is centered at origin
            normals_sphere.push_back(normal);

            // Assign a color (e.g., red for all vertices)
            colors_sphere.push_back(vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Example color
        }
    }

    // (Your index generation code remains the same)
    for (int lat = 0; lat < latitudeBands; lat++) {
         for (int lon = 0; lon < longitudeBands; lon++) {
             int first = lat * (longitudeBands + 1) + lon;
             int second = first + longitudeBands + 1;

             indices_sphere.push_back(first);
             indices_sphere.push_back(second);
             indices_sphere.push_back(first + 1);

             indices_sphere.push_back(second);
             indices_sphere.push_back(second + 1);
             indices_sphere.push_back(first + 1);
         }
    }
}

void setupSphereBuffers(GLuint vPositionLoc, GLuint vColorLoc, GLuint vNormalLoc) {
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    // Position Buffer
    glGenBuffers(1, &sphereVBO); // Reuse sphereVBO name, or use separate names
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, points_sphere.size() * sizeof(point4), points_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vPositionLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vPositionLoc);

    // Color Buffer
    GLuint sphereColorVBO; // Dedicated VBO for color
    glGenBuffers(1, &sphereColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereColorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors_sphere.size() * sizeof(vec4), colors_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vColorLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vColorLoc);

    // Normal Buffer
    GLuint sphereNormalVBO; // Dedicated VBO for normals
    glGenBuffers(1, &sphereNormalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereNormalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals_sphere.size() * sizeof(vec3), normals_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // Size 3 for vec3
    glEnableVertexAttribArray(vNormalLoc);

    // Index Buffer (IBO)
    glGenBuffers(1, &sphereIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_sphere.size() * sizeof(GLuint), indices_sphere.data(), GL_STATIC_DRAW);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


//---------------------------------------------------------------------
//
// init
//
void init()
{
    // 1. Initialize Physics Object (needs computeInitialPosition)
    //    computeInitialPosition uses gCameraEye, gCameraAt, gInitialFOVy, gZoomFactor, sceneWidth, sceneHeight
    float sphereGeneratedRadius = 0.5f; // Radius used in generateSphere
    vec3 initPos = computeInitialPosition(sphereGeneratedRadius);
    currentMaterial = plasticMaterial;

    // Initialize initialVelocity if not done globally
    initialVelocity = vec3(0.5f, 0.0f, 0.0f); // Example initial X-velocity

    bouncingObject = PhysicsObject(initPos, initialVelocity, vec3(0.0f), 1.0f); // Mass = 1.0f

    // 2. Load shaders and use the resulting shader program
    program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    
    materialSpecularIntensityLoc = glGetUniformLocation(program, "material.specularIntensity");
    materialShininessLoc = glGetUniformLocation(program, "material.shininess");

     
    GLint eyePosLoc = glGetUniformLocation(program, "eyePosition");
    if (eyePosLoc != -1) {
        glUniform3f(eyePosLoc, 0.0f, 0.0f, 0.0f);
    } else {
        std::cerr << "Warning: 'eyePosition' uniform not found in shader." << std::endl;
    }

    // 3. Setup vertex data and buffers for the sphere
    GLuint vPositionLoc = glGetAttribLocation(program, "vPosition");
    GLuint vColorLoc = glGetAttribLocation(program, "vColor");
    GLuint vNormalLoc = glGetAttribLocation(program, "vNormal");
    
    generateSphere(sphereGeneratedRadius);
    setupSphereBuffers(vPositionLoc, vColorLoc, vNormalLoc);

    // 4. Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    // 5. Set up and send the initial Projection matrix
    updateProjection(); // This now sets the perspective projection


    // 6. Set up Lighting uniforms (ensure your Light class or direct glUniform calls are correct)
    vec3 lightColor(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.3f;
    vec3 lightDirection(1.0f, 1.0f, 0.0f); // Example direction
    float diffuseIntensity = 0.7f;

    // Uniform locations for the DirectionalLight struct in the shader
    GLint ambientColorLoc = glGetUniformLocation(program, "directionalLight.color");
    GLint ambientIntensityLoc = glGetUniformLocation(program, "directionalLight.ambientIntensity");
    GLint lightDirectionLoc = glGetUniformLocation(program, "directionalLight.direction");
    GLint diffuseIntensityLoc = glGetUniformLocation(program, "directionalLight.diffuseIntensity");

    // Check if locations are valid (not -1) before using them
    if (ambientColorLoc != -1) glUniform3fv(ambientColorLoc, 1, &lightColor[0]);
    if (ambientIntensityLoc != -1) glUniform1f(ambientIntensityLoc, ambientStrength);
    if (lightDirectionLoc != -1) glUniform3fv(lightDirectionLoc, 1, &lightDirection[0]);
    if (diffuseIntensityLoc != -1) glUniform1f(diffuseIntensityLoc, diffuseIntensity);
    // 7. Set OpenGL states
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0); // Set background color (black)
}

//---------------------------------------------------------------------
//
// display
//
void display(void) {

    // 1. Ensure Depth Testing is Enabled at the Start of Each Frame
    glEnable(GL_DEPTH_TEST);

    // 2. Set the Depth Function (Default is GL_LESS, but being explicit is good)
    glDepthFunc(GL_LESS);

    // 3. Clear Both Color and Depth Buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
/*
    // 4. Render Background
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLuint backgroundProgram = InitShader("background_vshader.glsl", "background_fshader.glsl");
    glUseProgram(backgroundProgram);
    glBindVertexArray(backgroundVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(backgroundProgram, "texture1"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
*/
    // 5. Render the Physics Object (Sphere)
    glPolygonMode(GL_FRONT_AND_BACK, drawingMode);
    glUseProgram(program);
    
    if (materialSpecularIntensityLoc != -1 && materialShininessLoc != -1) {
        currentMaterial.UseMaterial(materialSpecularIntensityLoc, materialShininessLoc);
    }

    
    bouncingObject.update(deltaTime);
     
    // Create View Matrix (Camera)
    mat4 view_matrix = LookAt(gCameraEye, gCameraAt, gCameraUp);

    mat4 model_view = Translate(bouncingObject.position.x, bouncingObject.position.y, 0.1f) *
        RotateY(Theta[Yaxis]) *
        RotateZ(Theta[Zaxis]);

    float sphereScale = 0.48f; // Scale of the sphere
    mat4 model_matrix = model_view * Scale(sphereScale, sphereScale, sphereScale);

    // Calculate the final ModelView matrix (View * Model)
    mat4 final_model_view_matrix = view_matrix * model_matrix;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, final_model_view_matrix);
    // Draw the sphere
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO); // Good practice to rebind IBO
    glDrawElements(GL_TRIANGLES, indices_sphere.size(), GL_UNSIGNED_INT, 0);

    glFinish();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;
    switch (key) {
    case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
        exit(EXIT_SUCCESS);
        break;
    case GLFW_KEY_I:
    {
        float sphereGeneratedRadius = 0.5f; // Consistent with generateSphere
        vec3 pos = computeInitialPosition(sphereGeneratedRadius);
        bouncingObject.position = pos;
        bouncingObject.velocity = initialVelocity; // Reset to stored initial velocity
        bouncingObject.acceleration = vec3(0.0f, 0.0f, 0.0f); // Reset acceleration
        break;
    }
    case GLFW_KEY_O:
    {
        //The user should be able to turn off the specular, diffuse and ambient components one by one when desired
        
        break;
    }
    case GLFW_KEY_S:
    {
        //The user should be able to switch between Gouraud and Phong shading options
        
        break;
    }
    case GLFW_KEY_L:
    {
        //The user should be able to keep the light source fixed in position or move with the object
        break;
    }
    case GLFW_KEY_M:
    {
        // Toggle between plastic and metallic
        static bool isPlastic = true; // Keep track of current material state
        if (isPlastic) {
            currentMaterial = metallicMaterial;
            std::cout << "Switched to Metallic Material" << std::endl;
        } else {
            currentMaterial = plasticMaterial;
            std::cout << "Switched to Plastic Material" << std::endl;
        }
        isPlastic = !isPlastic;
        // No need to call UseMaterial here, it's called in display() every frame.
        break;
    }
    case GLFW_KEY_Z: // Zoom In
        {
            gZoomFactor *= (1.0f - gZoomStepFactor); // Decrease zoom factor (e.g., 0.9)
            if (gZoomFactor < 0.05f) gZoomFactor = 0.05f; // Set a minimum zoom limit
            updateProjection();
            break;
        }
    
    case GLFW_KEY_W:// Zoom Out
        {
            gZoomFactor *= (1.0f + gZoomStepFactor); // Increase zoom factor (e.g., 1.1)
            if (gZoomFactor > 20.0f) gZoomFactor = 20.0f; // Set a maximum zoom limit
            updateProjection();
            break;
        }
    case GLFW_KEY_H:
        std::cout << "CONTROL INSTRUCTIONS\n\nH -- Help\nI -- Initialize the pose\nO -- Change light mode (ambient - diffuse - specular)\nS -- Change shading mode (Gouraud and Phong shading)\nL -- Initialize the light position\nM -- Change material mode\nZ -- Zoom in\nW -- Zoom out\nMouse left click -- Change the drawing mode (Fill & Line)\nQ -- Quit(exit)\n" << std::endl;
        break;
    }
        
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            drawingMode = (drawingMode == GL_LINE) ? GL_FILL : GL_LINE;
            glPolygonMode(GL_FRONT_AND_BACK, drawingMode);
            break;
        }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height); // Set the viewport
    sceneWidth = width;  // Update the global width variable
    sceneHeight = height; // Update the global height variable
    if (height > 0) { // Prevent division by zero for aspect ratio if height is 0
         updateProjection(); // Update the projection matrix for the new aspect ratio
    }
}
// Simulate the movement again
vec3 computeInitialPosition(float objectRadiusUnscaled) {
    // The sphere is generated with radius objectRadiusUnscaled (e.g., 0.5f).
    // In the display function, it's scaled by sphereScale (e.g., 0.48f).
    float actualVisualRadius = objectRadiusUnscaled * 0.48f; // This is the apparent radius after scaling in display()

    // Calculate the world coordinates for "top-left" at the Z=0 plane.
    // This is the plane where the object will be initially placed.

    // Distance from the camera's eye to the object's Z-plane (Z=0).
    // Assumes gCameraEye.z > 0 and object is at Z=0.
    float distance_to_object_plane = gCameraEye.z;

    // A basic check if the camera is too close or behind the object plane.
    if (distance_to_object_plane <= actualVisualRadius) { // Check against radius for better safety
        std::cerr << "Warning: Camera might be too close to or behind the object plane "
                  << "for accurate top-left initial position calculation. "
                  << "Using a default fallback position." << std::endl;
        // Fallback to a somewhat top-left-ish default if calculation is problematic
        return vec3(-0.75f, 0.75f, 0.0f);
    }

    // Get the current Field of View (FOV) based on initial FOV and zoom factor.
    GLfloat current_fovy_degrees = gInitialFOVy * gZoomFactor;
    // Clamp FOV to avoid extreme values, consistent with updateProjection()
    if (current_fovy_degrees < 1.0f) current_fovy_degrees = 1.0f;
    if (current_fovy_degrees > 120.0f) current_fovy_degrees = 120.0f;

    // Get the current aspect ratio.
    float aspect = 1.0f; // Default aspect ratio
    if (sceneHeight > 0) {
        aspect = (float)sceneWidth / (float)sceneHeight;
    }

    // Calculate the height and width of the view frustum at the object's Z-plane.
    // The gCameraAt.y and gCameraAt.x will be the center of this view at that distance.
    float half_fovy_rad = degreesToRadians(current_fovy_degrees / 2.0f); // THIS LINE SHOULD NOW WORK
    float view_height_at_object_plane = 2.0f * distance_to_object_plane * tan(half_fovy_rad);
    float view_width_at_object_plane = view_height_at_object_plane * aspect;

    // Determine the world coordinates of the frustum edges at the object's Z-plane (Z=0),
    // relative to the camera's look-at point (gCameraAt).
    float world_top_y_boundary = gCameraAt.y + view_height_at_object_plane / 2.0f;
    float world_left_x_boundary = gCameraAt.x - view_width_at_object_plane / 2.0f;

    // Place the *center* of the sphere such that its top edge aligns with world_top_y_boundary
    // and its left edge aligns with world_left_x_boundary.
    vec3 initialPosition;
    initialPosition.x = world_left_x_boundary + actualVisualRadius;
    initialPosition.y = world_top_y_boundary - actualVisualRadius;
    initialPosition.z = 0.0f; // Sphere starts at the Z=0 plane

    return initialPosition;
}

void createAndBindBuffer(const void* data, size_t dataSize, GLuint& vao, GLuint& vbo, GLuint vPosition, const GLuint* indicesData = nullptr, size_t indicesSize = 0) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, data);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vPosition);

    if (indicesData) {
        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indicesData, GL_STATIC_DRAW);
        if (vao == sphereVAO) sphereIBO = ibo;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

//---------------------------------------------------------------------
//
// main
//

int main()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(sceneWidth, sceneHeight, "Bouncing Objects", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    init();

    double frameRate = 120.0;
    double currentTime, previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    
  
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        Theta[Axis] += rotationSpeed * deltaTime;
        if (Theta[Axis] > 360.0f) Theta[Axis] -= 360.0f;
        else if (Theta[Axis] < 0.0f) Theta[Axis] += 360.0f;
  
        display();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
