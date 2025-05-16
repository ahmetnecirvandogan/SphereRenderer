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


GLuint program;
GLuint texture; // Texture ID
//GLuint backgroundVAO, backgroundVBO; // VAO and VBO for background quad
double frameRate = 120;
double deltaTime = 1.0 / frameRate;
vec3 initialVelocity;
typedef vec4  point4;
void bindObject(GLuint vPosition);

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
float backgroundVertices[] = {
    // positions         // texture coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};
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
// About the scene
const int sceneWidth = 1200;
const int sceneHeight = 600;

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

// Color uniform location
GLuint colorLocation;
vec4 colorFirst(1.0, 0.0f, 0.0f, 1.0f);  // Red
vec4 colorSecond(0.6f, 0.3f, 0.0f, 1.0f);  // Brown
vec4 currentColor = colorFirst;
bool isRed = true;

Light lightSource;
GLuint vPosition;

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
    //Background initialization
    //loadTexture("toy-story-background.jpg");
    //setupBackground();

    float objectSize = 0.5f;
    vec3 initPos = computeInitialPosition(objectSize);
    vec3 initVel(0.5f, 0.0f, 0.0f); // Set initial x-velocity here
    initialVelocity = initVel; // Store the initial velocity
    vec3 initAcc(0.0f);
    float initMass = 1.0f;
    bouncingObject = PhysicsObject(initPos, initVel, initAcc, initMass);
    bouncingObject.position = initPos;

    // Load shaders and use the resulting shader program
    program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // bind the object to the shader
    vPosition = glGetAttribLocation(program, "vPosition");
    bindObject(vPosition);  // pass it in

    // Retrieve color uniform variable locations
    colorLocation = glGetUniformLocation(program, "color");
    glUniform4fv(colorLocation, 1, currentColor);

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    // Set projection matrix
    mat4  projection;


    float aspect = (float)sceneWidth / (float)sceneHeight;
    float viewHeight = 2.0f;
    float viewWidth = viewHeight * aspect;
    float top = viewHeight / 2.0f;
    float bottom = -top;
    float right = viewWidth / 2.0f;
    float left = -right;
    projection = Ortho(left, right, bottom, top, -1.0, 1.0);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    // Lighting uniforms
    vec3 lightColor(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.3f;
    vec3 lightDirection(1.0f, 1.0f, 0.0f);
    float diffuseIntensity = 0.7f;
    
    GLint ambientColorLoc = glGetUniformLocation(program, "directionalLight.color");
    GLint ambientIntensityLoc = glGetUniformLocation(program, "directionalLight.ambientIntensity");
    GLint lightDirectionLoc = glGetUniformLocation(program, "directionalLight.direction");
    GLint diffuseIntensityLoc = glGetUniformLocation(program, "directionalLight.diffuseIntensity");

    glUniform3fv(ambientColorLoc, 1, &lightColor[0]);
    glUniform1f(ambientIntensityLoc, ambientStrength);
    glUniform3fv(lightDirectionLoc, 1, &lightDirection[0]);
    glUniform1f(diffuseIntensityLoc, diffuseIntensity);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

//---------------------------------------------------------------------
//
// display
//
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int Axis = Yaxis;
GLfloat Theta[NumAxes] = { 0.0, 0.0, 0.0 };
float rotationSpeed = 50.0f;

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
    bouncingObject.update(deltaTime);

    mat4 model_view = Translate(bouncingObject.position.x, bouncingObject.position.y, 0.1f) *
        RotateY(Theta[Yaxis]) *
        RotateZ(Theta[Zaxis]);

    float sphereScale = 0.48f;
    mat4 sphere_model_view = model_view * Scale(sphereScale, sphereScale, sphereScale);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, sphere_model_view);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
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
        float objectSize = 0.48f;
        vec3 pos = computeInitialPosition(objectSize);
        bouncingObject.position = pos;
        bouncingObject.velocity = initialVelocity; // Set velocity to the stored initial velocity
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
        //Define at least two options for material properties: either plastic or metallic (use the key M to toggle).
        break;
    }
    case GLFW_KEY_Z:
    {
        //The user should be able to zoom in and out (use the keys Z and W).
        break;
    }
    
    case GLFW_KEY_W:
    {
        //The user should be able to zoom in and out (use the keys Z and W).
        break;
    }

    case GLFW_KEY_C:
        isRed = !isRed;
        currentColor = isRed ? colorFirst : colorSecond;
        glUseProgram(program);
        glUniform4fv(colorLocation, 1, currentColor);
        break;
    case GLFW_KEY_H:
        std::cout << "i -- initialize the pose (top left corner of the window)\nc-- switch between two colors(of your choice), which is used to draw lines or triangles.\nh -- help; print explanation of your input control(simply to the command line)\nq -- quit(exit) the program" << std::endl;
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
    glViewport(0, 0, width, height);
}


// Simulate the movement again
vec3 computeInitialPosition(float objectSize)
{
    float aspect = (float)sceneWidth / (float)sceneHeight;
    float viewHeight = 2.0f;
    float viewWidth = viewHeight * aspect;
    float top = viewHeight / 2.0f;
    float bottom = -top;
    float right = viewWidth / 2.0f;
    float left = -right;

    float halfObjectSize = objectSize / 2.0f;

    vec3 initialPosition = vec3(left + halfObjectSize, top - halfObjectSize, 0.0f);
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

void bindObject(GLuint vPositionLoc) { // Rename parameter for clarity
     generateSphere(0.5f);
     GLuint vColorLoc = glGetAttribLocation(program, "vColor"); // Get color location
     GLuint vNormalLoc = glGetAttribLocation(program, "vNormal"); // Get normal location
     setupSphereBuffers(vPositionLoc, vColorLoc, vNormalLoc); // Call the new setup function
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
