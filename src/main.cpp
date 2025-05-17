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
std::vector<vec2> texCoords_sphere;
GLuint sphereVAO, sphereVBO, sphereIBO; // IBO for sphere

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;
mat4 gProjectionMatrix; // Global projection matrix
float gZoomFactor = 1.0f; // Global zoom factor
const float gZoomStepFactor = 0.1f;

// Color uniform location
GLuint colorLocation;
vec4 colorFirst(1.0, 0.0f, 0.0f, 1.0f);  // Red
vec4 colorSecond(0.6f, 0.3f, 0.0f, 1.0f);  // Brown
vec4 currentColor = colorFirst;
bool isRed = true;

Light lightSource;
GLuint vPosition;

struct Image {
    unsigned char* data;
    int width;
    int height;
    int max_val;
};

Image loadPPM(const char* filename) {
    FILE* fp;
    char buff[100];
    Image img;
    int c;

    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        exit(1);
    }

    // Read magic number
    fscanf(fp, "%s", buff);
    if (buff[0] != 'P' || buff[1] != '3') {
        fprintf(stderr, "Invalid PPM file format (must be P3)\n");
        exit(1);
    }

    // Skip comments
    c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n');
        c = getc(fp);
    }
    ungetc(c, fp);

    // Read dimensions and max value
    fscanf(fp, "%d %d", &img.width, &img.height);
    fscanf(fp, "%d", &img.max_val);

    // Allocate memory
    img.data = (unsigned char*)malloc(img.width * img.height * 3);
    if (!img.data) {
        fprintf(stderr, "Unable to allocate memory for image data\n");
        exit(1);
    }

    // Read pixel data
    int pixel_index = 0;
    for (int i = 0; i < img.width * img.height; i++) {
        int r, g, b;
        fscanf(fp, "%d %d %d", &r, &g, &b);
        img.data[pixel_index++] = (unsigned char)r;
        img.data[pixel_index++] = (unsigned char)g;
        img.data[pixel_index++] = (unsigned char)b;
    }

    fclose(fp);
    return img;
}
//For the Background
void loadTexture(const char* filename) {
    Image img = loadPPM(filename); // Use your custom loader

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Use GL_RGB for PPM P3 format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(img.data); // Free the image data after uploading to the GPU
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


// generation of the sphere
void generateSphere(float radius) {
    points_sphere.clear();
    indices_sphere.clear();
    normals_sphere.clear();
    colors_sphere.clear();
    std::vector<vec2> texCoords_sphere; // Add texture coordinates vector

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

            vec3 normal = normalize(vec3(x, y, z));
            normals_sphere.push_back(normal);

            colors_sphere.push_back(vec4(1.0f, 0.0f, 0.0f, 1.0f));

            // Generate texture coordinates
            vec2 texCoord = vec2(1.0f - ((float)lon / longitudeBands), (float)lat / latitudeBands); // Adjusted mapping
            texCoords_sphere.push_back(texCoord);
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

void setupSphereBuffers(GLuint vPositionLoc, GLuint vColorLoc, GLuint vNormalLoc, GLuint vTexCoordLoc) {
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    // Position Buffer
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, points_sphere.size() * sizeof(point4), points_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vPositionLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vPositionLoc);

    // Color Buffer
    GLuint sphereColorVBO;
    glGenBuffers(1, &sphereColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereColorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors_sphere.size() * sizeof(vec4), colors_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vColorLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vColorLoc);

    // Normal Buffer
    GLuint sphereNormalVBO;
    glGenBuffers(1, &sphereNormalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereNormalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals_sphere.size() * sizeof(vec3), normals_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vNormalLoc);

    // Texture Coordinate Buffer
    GLuint sphereTexCoordVBO; // Dedicated VBO for texture coordinates
    glGenBuffers(1, &sphereTexCoordVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereTexCoordVBO);
    glBufferData(GL_ARRAY_BUFFER, texCoords_sphere.size() * sizeof(vec2), texCoords_sphere.data(), GL_STATIC_DRAW); // Use vec2 for tex coords
    glVertexAttribPointer(vTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); // Size 2 for vec2
    glEnableVertexAttribArray(vTexCoordLoc);

    // Index Buffer (IBO)
    glGenBuffers(1, &sphereIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_sphere.size() * sizeof(GLuint), indices_sphere.data(), GL_STATIC_DRAW);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Helper function to update the projection matrix
void updateProjection() {
    if (sceneHeight == 0) sceneHeight = 1; // Prevent division by zero
    float aspect = (float)sceneWidth / (float)sceneHeight;
    float baseViewHeight = 2.0f; // Base height when zoomFactor is 1.0

    // When gZoomFactor < 1.0 (zoom in), currentViewHeight is smaller -> smaller ortho box -> objects larger
    // When gZoomFactor > 1.0 (zoom out), currentViewHeight is larger -> larger ortho box -> objects smaller
    float currentViewHeight = baseViewHeight * gZoomFactor;
    float currentViewWidth = currentViewHeight * aspect;

    float top = currentViewHeight / 2.0f;
    float bottom = -top;
    float right = currentViewWidth / 2.0f;
    float left = -right;

    gProjectionMatrix = Ortho(left, right, bottom, top, -1.0, 1.0);
    
    // Ensure the correct shader program is active before updating its uniforms.
    // This is important if you switch between shader programs elsewhere.
    glUseProgram(program);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, gProjectionMatrix);
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
    
    bindObject(vPosition); // This will call generateSphere and setupSphereBuffers
 
    // Load your PPM texture
    loadTexture("basketball.ppm");

    // Retrieve color uniform variable locations
    colorLocation = glGetUniformLocation(program, "color");
    glUniform4fv(colorLocation, 1, currentColor);

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    updateProjection();

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
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0); // Use texture unit 0
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, "textureSampler"), 0); // Set texture uniform to unit 0

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

void bindObject(GLuint vPositionLoc) {
    generateSphere(0.5f);
    GLuint vColorLoc = glGetAttribLocation(program, "vColor");
    GLuint vNormalLoc = glGetAttribLocation(program, "vNormal");
    GLuint vTexCoordLoc = glGetAttribLocation(program, "vTexCoord"); // Get texture coordinate location
    setupSphereBuffers(vPositionLoc, vColorLoc, vNormalLoc, vTexCoordLoc); // Pass texture coordinate location
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
