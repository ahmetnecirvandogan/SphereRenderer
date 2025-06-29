#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Angel.h"
#include "PhysicsObject.h"
#include <vector>
#include <fstream>
#include "light.h"
#include "Material.h"
#include "ppm_loader.h" // For loading PPM images
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Helper functions
inline float degreesToRadians(float degrees) {
    return degrees * (float(M_PI) / 180.0f);
}
mat3 extract_mat3_from_mat4(const mat4& m) {
    return mat3(
        vec3(m[0].x, m[0].y, m[0].z),
        vec3(m[1].x, m[1].y, m[1].z),
        vec3(m[2].x, m[2].y, m[2].z)
    );
}

GLuint program;

// Texture IDs
GLuint earthTextureID = 0;
GLuint basketballTextureID = 0;
GLuint synthetic1DTexID = 0;

// For sphere texture coordinates
std::vector<vec2> texCoords_sphere;

// For 1D Texture Mapping Bonus
int g_activeTextureConfig = 0; // 0: Earth (2D), 1: Basketball (2D), 2: Synthetic (1D)
vec4 g_1DTexturePlaneParams = vec4(0.0f, 1.0f, 0.0f, 0.0f); // Plane: y=0
float g_1DTextureStripeFrequency = 10.0f;

// Uniform locations
GLuint u_ModelMatrixLoc = GL_INVALID_INDEX;
GLuint u_1DTexturePlaneLoc = GL_INVALID_INDEX;
GLuint u_1DTextureStripeScaleLoc = GL_INVALID_INDEX;
GLuint u_currentTextureTypeLoc = GL_INVALID_INDEX;
GLuint textureSampler2DLoc = GL_INVALID_INDEX;
GLuint textureSampler1DLoc = GL_INVALID_INDEX;

double frameRate = 120;
double deltaTime = 1.0 / frameRate;
vec3 initialVelocity;
typedef vec4  point4;

int sceneWidth = 1200;
int sceneHeight = 600;

PhysicsObject bouncingObject;
vec3 computeInitialPosition(float objectSize);
std::vector<vec3> normals_sphere;
std::vector<vec4> colors_sphere;

enum DisplayMode {
    MODE_SHADING = 0,
    MODE_SHADING_WITH_SHADOW = 1,
    MODE_WIREFRAME = 2,
    MODE_TEXTURE = 3
};
DisplayMode currentDisplayMode = MODE_SHADING;
GLuint displayModeLoc;

const float floorLevel = -1.0f;
GLuint u_isShadowPassLoc;
GLuint u_shadowColorLoc;

const int latitudeBands = 50;
const int longitudeBands = 50;
std::vector<point4> points_sphere;
std::vector<GLuint> indices_sphere;
GLuint sphereVAO, sphereVBO, sphereIBO;

GLuint  ModelView, Projection;
vec3 gCameraEye = vec3(0.0f, 0.5f, 3.0f);
vec3 gCameraAt = vec3(0.0f, 0.0f, 0.0f);
vec3 gCameraUp = vec3(0.0f, 1.0f, 0.0f);
GLfloat gInitialFOVy = 45.0f;
GLfloat gZNear = 0.1f;
GLfloat gZFar = 100.0f;

mat4 gProjectionMatrix;
float gZoomFactor = 1.0f;
const float gZoomStepFactor = 0.1f;

Material plasticMaterial(0.5f, 32.0f);
Material metallicMaterial(0.8f, 128.0f);
Material currentMaterial;

int gShadingMode = 1; // 1 for Phong (default), 0 for Gouraud
GLuint shadingModeLoc;

bool gIsLightFixed = true;
vec3 gFixedLightDirection_World = normalize(vec3(-0.5f, -0.5f, 1.0f));
vec3 gObjectLocalLightDirection = normalize(vec3(0.0f, 0.0f, 1.0f));
GLuint gLightDirectionLoc;

GLuint materialSpecularIntensityLoc;
GLuint materialShininessLoc;
GLuint enableAmbientLoc, enableDiffuseLoc, enableSpecularLoc;
float enableAmbientVal = 1.0f;
float enableDiffuseVal = 1.0f;
float enableSpecularVal = 1.0f;
int gLightComponentToggleIndex = 0;

enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int Axis = Yaxis;
GLfloat Theta[NumAxes] = { 0.0, 0.0, 0.0 };
float rotationSpeed = 50.0f;

void updateProjection() {
    if (sceneHeight == 0) sceneHeight = 1;
    float aspect = (float)sceneWidth / (float)sceneHeight;
    GLfloat current_fovy = gInitialFOVy * gZoomFactor;
    if (current_fovy < 1.0f) current_fovy = 1.0f;
    if (current_fovy > 120.0f) current_fovy = 120.0f;
    gProjectionMatrix = Perspective(current_fovy, aspect, gZNear, gZFar);
    if (program && Projection != GL_INVALID_INDEX) {
        glUseProgram(program);
        glUniformMatrix4fv(Projection, 1, GL_TRUE, gProjectionMatrix);
    }
}

void generateSphere(float radius) {
    points_sphere.clear();
    indices_sphere.clear();
    normals_sphere.clear();
    colors_sphere.clear();
    texCoords_sphere.clear();

    for (int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * float(M_PI) / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        for (int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2.0f * float(M_PI) / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            points_sphere.push_back(point4(radius * x, radius * y, radius * z, 1.0));
            normals_sphere.push_back(normalize(vec3(x, y, z)));
            colors_sphere.push_back(vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Default color red
            float s = (float)lon / longitudeBands;
            float t = 1.0f - (float)lat / latitudeBands;
            texCoords_sphere.push_back(vec2(s, t));
        }
    }
    for (int lat = 0; lat < latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
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

    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, points_sphere.size() * sizeof(point4), points_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vPositionLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vPositionLoc);

    GLuint sphereColorVBO; // This can be a local variable if not used elsewhere
    glGenBuffers(1, &sphereColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereColorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors_sphere.size() * sizeof(vec4), colors_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vColorLoc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vColorLoc);

    GLuint sphereNormalVBO; // This can be a local variable
    glGenBuffers(1, &sphereNormalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereNormalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals_sphere.size() * sizeof(vec3), normals_sphere.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(vNormalLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(vNormalLoc);

    if (vTexCoordLoc != GL_INVALID_INDEX && !texCoords_sphere.empty()) {
        GLuint sphereTexCoordVBO; // This can be a local variable
        glGenBuffers(1, &sphereTexCoordVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereTexCoordVBO);
        glBufferData(GL_ARRAY_BUFFER, texCoords_sphere.size() * sizeof(vec2), texCoords_sphere.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(vTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(vTexCoordLoc);
    } else if (vTexCoordLoc != GL_INVALID_INDEX) {
         std::cerr << "Warning: vTexCoordLoc is valid for shader, but texCoords_sphere is empty." << std::endl;
    }

    glGenBuffers(1, &sphereIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO); // This IBO binding becomes part of sphereVAO's state
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_sphere.size() * sizeof(GLuint), indices_sphere.data(), GL_STATIC_DRAW);

    glBindVertexArray(0); // Unbind VAO first
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Then unbind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO (though VAO 0 doesn't record IBO state in the same way)
}

void init()
{
    std::cout << "3.1 OpenGL Initialized!" << std::endl;

    float sphereGeneratedRadius = 0.5f;
    vec3 initPos = computeInitialPosition(sphereGeneratedRadius);
    currentMaterial = plasticMaterial;
    initialVelocity = vec3(0.5f, 0.0f, 0.0f);
    bouncingObject = PhysicsObject(initPos, initialVelocity, vec3(0.0f), 1.0f);

    program = InitShader("vshader.glsl", "fshader.glsl");
    if (program == 0) {
        std::cerr << "Failed to initialize shaders. Exiting." << std::endl;
        exit(EXIT_FAILURE);
    }
    glUseProgram(program);

    // Get standard uniform locations
    materialSpecularIntensityLoc = glGetUniformLocation(program, "material.specularIntensity");
    materialShininessLoc = glGetUniformLocation(program, "material.shininess");
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    shadingModeLoc = glGetUniformLocation(program, "shadingMode");
    displayModeLoc = glGetUniformLocation(program, "displayMode");
    GLint eyePosLoc = glGetUniformLocation(program, "eyePosition");
    gLightDirectionLoc = glGetUniformLocation(program, "directionalLight.direction");
    enableAmbientLoc = glGetUniformLocation(program, "enableAmbient");
    enableDiffuseLoc = glGetUniformLocation(program, "enableDiffuse");
    enableSpecularLoc = glGetUniformLocation(program, "enableSpecular");
    u_isShadowPassLoc = glGetUniformLocation(program, "u_isShadowPass");
    u_shadowColorLoc = glGetUniformLocation(program, "u_shadowColor");

    // Get attribute locations
    GLuint vPositionLoc = glGetAttribLocation(program, "vPosition");
    GLuint vColorLoc = glGetAttribLocation(program, "vColor");
    GLuint vNormalLoc = glGetAttribLocation(program, "vNormal");
    GLuint vTexCoordLoc = glGetAttribLocation(program, "aTexCoord");
    if(vTexCoordLoc == GL_INVALID_INDEX) { // Should be found by shader
        std::cerr << "Warning: Attribute 'aTexCoord' not found in vertex shader." << std::endl;
    }

    generateSphere(sphereGeneratedRadius);
    setupSphereBuffers(vPositionLoc, vColorLoc, vNormalLoc, vTexCoordLoc);

    updateProjection();

    // Set initial lighting uniforms
    if (eyePosLoc != -1) glUniform3fv(eyePosLoc, 1, gCameraEye);
    else std::cerr << "Warning: 'eyePosition' uniform not found." << std::endl;

    vec3 lightColorVal(1.0f, 1.0f, 1.0f);
    float ambientStrengthVal = 0.3f;
    float diffuseIntensityVal = 0.7f;
    GLint ambientColorLoc = glGetUniformLocation(program, "directionalLight.color");
    GLint ambientIntensityLoc = glGetUniformLocation(program, "directionalLight.ambientIntensity");
    GLint diffuseIntensityLoc = glGetUniformLocation(program, "directionalLight.diffuseIntensity");

    if(ambientColorLoc != -1) glUniform3fv(ambientColorLoc, 1, &lightColorVal[0]);
    if(ambientIntensityLoc != -1) glUniform1f(ambientIntensityLoc, ambientStrengthVal);
    if(diffuseIntensityLoc != -1) glUniform1f(diffuseIntensityLoc, diffuseIntensityVal);
    // gLightDirectionLoc is set per frame in display() as it might change

    if(enableAmbientLoc != -1) glUniform1f(enableAmbientLoc, enableAmbientVal);
    if(enableDiffuseLoc != -1) glUniform1f(enableDiffuseLoc, enableDiffuseVal);
    if(enableSpecularLoc != -1) glUniform1f(enableSpecularLoc, enableSpecularVal);
    if(shadingModeLoc != -1) glUniform1i(shadingModeLoc, gShadingMode);

    // --- Texture Loading ---
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Important for tightly packed PPM data

    std::cout << "Loading 2D textures..." << std::endl;
    PPMImage earthTexData = loadPPM("earth.ppm");
    if (earthTexData.isValid) {
        glGenTextures(1, &earthTextureID);
        glBindTexture(GL_TEXTURE_2D, earthTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, earthTexData.width, earthTexData.height, 0, GL_RGB, GL_UNSIGNED_BYTE, earthTexData.data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Using GL_LINEAR (no mipmap)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glGenerateMipmap(GL_TEXTURE_2D); // No mipmaps for this simplified setup
        std::cout << "Earth texture loaded. ID: " << earthTextureID << std::endl;
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        std::cerr << "Failed to load earth.ppm" << std::endl;
    }

    PPMImage basketballTexData = loadPPM("basketball.ppm");
    if (basketballTexData.isValid) {
        glGenTextures(1, &basketballTextureID);
        glBindTexture(GL_TEXTURE_2D, basketballTextureID);
        // *** CRITICAL FIX: Use basketballTexData here ***
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, basketballTexData.width, basketballTexData.height, 0, GL_RGB, GL_UNSIGNED_BYTE, basketballTexData.data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Using GL_LINEAR
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glGenerateMipmap(GL_TEXTURE_2D); // No mipmaps
        std::cout << "Basketball texture loaded. ID: " << basketballTextureID << std::endl;
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        std::cerr << "Failed to load basketball.ppm" << std::endl;
    }

    std::cout << "Creating 1D synthetic texture..." << std::endl;
    const int tex1DWidth = 256;
    const int stripePixelWidth = 32;
    unsigned char tex1DData[tex1DWidth * 3];
    for (int i = 0; i < tex1DWidth; ++i) {
        bool isFirstColorStripe = (i / stripePixelWidth) % 2 == 0;
        if (isFirstColorStripe) { tex1DData[i * 3 + 0] = 255; tex1DData[i * 3 + 1] = 0;   tex1DData[i * 3 + 2] = 0;   }
        else { tex1DData[i * 3 + 0] = 255; tex1DData[i * 3 + 1] = 255; tex1DData[i * 3 + 2] = 255; }
    }
    glGenTextures(1, &synthetic1DTexID);
    if (synthetic1DTexID == 0) {
        std::cerr << "Failed to generate 1D texture ID." << std::endl;
    } else {
        glBindTexture(GL_TEXTURE_1D, synthetic1DTexID);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, tex1DWidth, 0, GL_RGB, GL_UNSIGNED_BYTE, tex1DData);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_1D, 0);
        std::cout << "1D Synthetic Texture loaded. ID: " << synthetic1DTexID << std::endl;
    }
    // Get uniform locations for 1D/2D texturing
    u_ModelMatrixLoc = glGetUniformLocation(program, "u_ModelMatrix");
    u_1DTexturePlaneLoc = glGetUniformLocation(program, "u_1DTexturePlane");
    u_1DTextureStripeScaleLoc = glGetUniformLocation(program, "u_1DTextureStripeScale");
    u_currentTextureTypeLoc = glGetUniformLocation(program, "u_currentTextureType");
    textureSampler2DLoc = glGetUniformLocation(program, "textureSampler2D");
    textureSampler1DLoc = glGetUniformLocation(program, "textureSampler1D");

    // Warnings for new uniforms (can be kept or removed)
    if (u_ModelMatrixLoc == -1) std::cerr << "Warning: Uniform 'u_ModelMatrix' not found." << std::endl;
    if (u_1DTexturePlaneLoc == -1) std::cerr << "Warning: Uniform 'u_1DTexturePlane' not found." << std::endl;
    if (u_1DTextureStripeScaleLoc == -1) std::cerr << "Warning: Uniform 'u_1DTextureStripeScale' not found." << std::endl;
    if (u_currentTextureTypeLoc == -1) std::cerr << "Warning: Uniform 'u_currentTextureType' not found." << std::endl;
    if (textureSampler2DLoc == -1) std::cerr << "Warning: Uniform 'textureSampler2D' not found." << std::endl;
    if (textureSampler1DLoc == -1) std::cerr << "Warning: Uniform 'textureSampler1D' not found." << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearColor(0.0, 0.0, 0.0, 1.0); // Black background
    std::cout << "init() function completed." << std::endl;
}

void display(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    if (currentDisplayMode == MODE_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    int shaderDisplayModeForFS = 0;
    if (currentDisplayMode == MODE_SHADING_WITH_SHADOW) shaderDisplayModeForFS = 0;
    else if (currentDisplayMode == MODE_TEXTURE) shaderDisplayModeForFS = 2;
    else if (currentDisplayMode == MODE_SHADING) shaderDisplayModeForFS = 0;
    else if (currentDisplayMode == MODE_WIREFRAME) shaderDisplayModeForFS = 0;
    if (displayModeLoc != -1) glUniform1i(displayModeLoc, shaderDisplayModeForFS);

    if (materialSpecularIntensityLoc != -1 && materialShininessLoc != -1) {
        currentMaterial.UseMaterial(materialSpecularIntensityLoc, materialShininessLoc);
    }

    bouncingObject.update(deltaTime);

    mat4 view_matrix = LookAt(gCameraEye, gCameraAt, gCameraUp);
    mat4 sphere_model_matrix = Translate(bouncingObject.position.x, bouncingObject.position.y, bouncingObject.position.z) *
                           RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) * Scale(0.48f, 0.48f, 0.48f);

    if (u_ModelMatrixLoc != -1) glUniformMatrix4fv(u_ModelMatrixLoc, 1, GL_TRUE, sphere_model_matrix);

    int currentTexTypeShaderEnum = 0;
    if (g_activeTextureConfig == 2 && synthetic1DTexID != 0) {
        currentTexTypeShaderEnum = 1;
    }
    if (u_currentTextureTypeLoc != -1) glUniform1i(u_currentTextureTypeLoc, currentTexTypeShaderEnum);

    // Always set sampler uniforms to their respective texture units
    if (textureSampler2DLoc != -1) glUniform1i(textureSampler2DLoc, 0); // textureSampler2D uses unit 0
    if (textureSampler1DLoc != -1) glUniform1i(textureSampler1DLoc, 1); // textureSampler1D uses unit 1

    if (currentDisplayMode == MODE_TEXTURE) {
        if (currentTexTypeShaderEnum == 1) { // 1D Texture
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_1D, synthetic1DTexID);
            if (u_1DTexturePlaneLoc != -1) glUniform4fv(u_1DTexturePlaneLoc, 1, &g_1DTexturePlaneParams[0]);
            if (u_1DTextureStripeScaleLoc != -1) glUniform1f(u_1DTextureStripeScaleLoc, g_1DTextureStripeFrequency);
        } else { // 2D Texture
            glActiveTexture(GL_TEXTURE0);
            GLuint texToBind = 0;
            if (g_activeTextureConfig == 0 && earthTextureID != 0) texToBind = earthTextureID;
            else if (g_activeTextureConfig == 1 && basketballTextureID != 0) texToBind = basketballTextureID;
            else if (earthTextureID != 0) texToBind = earthTextureID;
            else if (basketballTextureID != 0) texToBind = basketballTextureID;
            glBindTexture(GL_TEXTURE_2D, texToBind);
        }
    } else {
        // When not in texture mode, unbind textures from our managed units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, 0);
        glActiveTexture(GL_TEXTURE0); // Reset active unit
    }

    vec3 world_light_direction_vector;
    if (gIsLightFixed) {
        world_light_direction_vector = gFixedLightDirection_World;
    } else {
        mat4 object_orientation_matrix = RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]);
        mat3 object_orientation_matrix_3x3 = extract_mat3_from_mat4(object_orientation_matrix);
        world_light_direction_vector = normalize(object_orientation_matrix_3x3 * gObjectLocalLightDirection);
    }
    vec3 currentLightDirection_ViewSpace = normalize(extract_mat3_from_mat4(view_matrix) * world_light_direction_vector);
    if (gLightDirectionLoc != -1) glUniform3fv(gLightDirectionLoc, 1, &currentLightDirection_ViewSpace[0]);

    if (u_isShadowPassLoc != -1) glUniform1i(u_isShadowPassLoc, 0);
    mat4 final_model_view_matrix = view_matrix * sphere_model_matrix;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, final_model_view_matrix);

    glBindVertexArray(sphereVAO);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO); // VAO remembers the EBO binding

    glDrawElements(GL_TRIANGLES, indices_sphere.size(), GL_UNSIGNED_INT, 0);
    
    if (currentDisplayMode == MODE_SHADING_WITH_SHADOW) {
        if (u_isShadowPassLoc != -1) glUniform1i(u_isShadowPassLoc, 1);
        if (u_shadowColorLoc != -1) glUniform4f(u_shadowColorLoc, 0.2f, 0.2f, 0.2f, 0.6f);
        
        vec3 L = normalize(world_light_direction_vector);
        mat4 directionalShadowMatrix = Angel::identity(); // Check mat.h if this causes console messages
        if (abs(L.y) > 0.0001f) {
            directionalShadowMatrix[0][1] = -L.x / L.y;
            directionalShadowMatrix[1][1] = 0.0f;
            directionalShadowMatrix[2][1] = -L.z / L.y;
            directionalShadowMatrix[0][3] = (L.x / L.y) * floorLevel;
            directionalShadowMatrix[1][3] = floorLevel;
            directionalShadowMatrix[2][3] = (L.z / L.y) * floorLevel;
        } else {
             if (u_isShadowPassLoc != -1) glUniform1i(u_isShadowPassLoc, 0);
        }
        
        mat4 model_matrix_for_shadow = directionalShadowMatrix * sphere_model_matrix;
        mat4 final_shadow_model_view = view_matrix * model_matrix_for_shadow;
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, final_shadow_model_view);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glDrawElements(GL_TRIANGLES, indices_sphere.size(), GL_UNSIGNED_INT, 0);
        
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        if (u_isShadowPassLoc != -1) glUniform1i(u_isShadowPassLoc, 0);
    }
    
    if(enableAmbientLoc != -1) glUniform1f(enableAmbientLoc, enableAmbientVal);
    if(enableDiffuseLoc != -1) glUniform1f(enableDiffuseLoc, enableDiffuseVal);
    if(enableSpecularLoc != -1) glUniform1f(enableSpecularLoc, enableSpecularVal);

    // glFinish(); // Generally not needed and can hurt performance
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    switch (key) {
    case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
        std::cout << "OpenGL Process Done!" << std::endl;
        exit(EXIT_SUCCESS);
        break;
    case GLFW_KEY_I:
    {
        g_activeTextureConfig = (g_activeTextureConfig + 1) % 3;
        if (g_activeTextureConfig == 0) std::cout << "Switched to Earth Texture (2D)" << std::endl;
        else if (g_activeTextureConfig == 1) std::cout << "Switched to Basketball Texture (2D)" << std::endl;
        else if (g_activeTextureConfig == 2) std::cout << "Switched to Synthetic 1D Texture" << std::endl;
        break;
    }
    case GLFW_KEY_H:
        std::cout << "CONTROL INSTRUCTIONS:\n"
                  << "H -- Help\n"
                  << "I -- Toggle Texture (Earth 2D / Basketball 2D / Synthetic 1D)\n"
                  << "O -- Toggle Lighting Components (Ambient/Diffuse/Specular)\n"
                  << "S -- Toggle Shading Mode (Gouraud/Phong)\n"
                  << "L -- Toggle Light Position (Fixed/With Object)\n"
                  << "M -- Toggle Material (Plastic/Metallic)\n"
                  << "T -- Toggle Display Mode (Shading/Shading+Shadow/Wireframe/Texture)\n"
                  << "Z -- Zoom In\n"
                  << "W -- Zoom Out\n"
                  << "R -- Reset the object position\n"
                  << "Q / ESC -- Quit\n" << std::endl;
        break;
    case GLFW_KEY_R:
    {
        float sphereGeneratedRadius = 0.5f;
        vec3 pos = computeInitialPosition(sphereGeneratedRadius);
        bouncingObject.position = pos;
        bouncingObject.velocity = initialVelocity;
        bouncingObject.acceleration = vec3(0.0f, 0.0f, 0.0f);
        std::cout << "Object position reset." << std::endl;
        break;
    }
    case GLFW_KEY_O:
        {
            switch (gLightComponentToggleIndex) {
                case 0: enableAmbientVal = 1.0f - enableAmbientVal; std::cout << "Toggled Ambient Light to: " << (enableAmbientVal > 0.5f ? "ON" : "OFF") << std::endl; break;
                case 1: enableDiffuseVal = 1.0f - enableDiffuseVal; std::cout << "Toggled Diffuse Light to: " << (enableDiffuseVal > 0.5f ? "ON" : "OFF") << std::endl; break;
                case 2: enableSpecularVal = 1.0f - enableSpecularVal; std::cout << "Toggled Specular Light to: " << (enableSpecularVal > 0.5f ? "ON" : "OFF") << std::endl; break;
            }
            gLightComponentToggleIndex = (gLightComponentToggleIndex + 1) % 3;
            std::cout << "  Current Light States -> Ambient: " << (enableAmbientVal > 0.5f ? "ON" : "OFF")
                      << ", Diffuse: " << (enableDiffuseVal > 0.5f ? "ON" : "OFF")
                      << ", Specular: " << (enableSpecularVal > 0.5f ? "ON" : "OFF") << std::endl;
            std::cout << "  Next 'O' press will toggle: "
                      << (gLightComponentToggleIndex == 0 ? "Ambient" : (gLightComponentToggleIndex == 1 ? "Diffuse" : "Specular"))
                      << std::endl;
            break;
        }
    case GLFW_KEY_S:
    {
        gShadingMode = 1 - gShadingMode;
        std::cout << "Switched to " << (gShadingMode == 0 ? "Gouraud" : "Phong") << " Shading" << std::endl;
        if (shadingModeLoc != -1) { glUseProgram(program); glUniform1i(shadingModeLoc, gShadingMode);}
        break;
    }
    case GLFW_KEY_L:
    {
        gIsLightFixed = !gIsLightFixed;
        std::cout << "Light source: " << (gIsLightFixed ? "FIXED in world space" : "MOVES WITH OBJECT") << std::endl;
        break;
    }
    case GLFW_KEY_M:
    {
        static bool isPlastic = true;
        if (isPlastic) { currentMaterial = metallicMaterial; std::cout << "Switched to Metallic Material" << std::endl;}
        else { currentMaterial = plasticMaterial; std::cout << "Switched to Plastic Material" << std::endl;}
        isPlastic = !isPlastic;
        break;
    }
    case GLFW_KEY_Z: { gZoomFactor *= (1.0f - gZoomStepFactor); if (gZoomFactor < 0.05f) gZoomFactor = 0.05f; updateProjection(); break; }
    case GLFW_KEY_W: { gZoomFactor *= (1.0f + gZoomStepFactor); if (gZoomFactor > 20.0f) gZoomFactor = 20.0f; updateProjection(); break; }
    case GLFW_KEY_T:
    {
        currentDisplayMode = static_cast<DisplayMode>((static_cast<int>(currentDisplayMode) + 1) % 4);
        switch (currentDisplayMode) {
            case MODE_SHADING: std::cout << "Display Mode: Shading" << std::endl; break;
            case MODE_SHADING_WITH_SHADOW: std::cout << "Display Mode: Shading with Shadow" << std::endl; break;
            case MODE_WIREFRAME: std::cout << "Display Mode: Wireframe" << std::endl; break;
            case MODE_TEXTURE: std::cout << "Display Mode: Texture" << std::endl; break;
        }
        break;
    }
    default: break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    sceneWidth = width;
    sceneHeight = height;
    if (height > 0) {
         updateProjection();
    }
}

vec3 computeInitialPosition(float objectRadiusUnscaled) {
    float actualVisualRadius = objectRadiusUnscaled * 0.48f;
    float distance_to_object_plane = gCameraEye.z;
    if (distance_to_object_plane <= actualVisualRadius) {
        std::cerr << "Warning: Camera might be too close for accurate top-left initial position. Using fallback." << std::endl;
        return vec3(-0.75f, 0.75f, 0.0f);
    }
    GLfloat current_fovy_degrees = gInitialFOVy * gZoomFactor;
    if (current_fovy_degrees < 1.0f) current_fovy_degrees = 1.0f;
    if (current_fovy_degrees > 120.0f) current_fovy_degrees = 120.0f;
    float aspect = (sceneHeight > 0) ? (float)sceneWidth / (float)sceneHeight : 1.0f;
    float half_fovy_rad = degreesToRadians(current_fovy_degrees / 2.0f);
    float view_height_at_object_plane = 2.0f * distance_to_object_plane * tan(half_fovy_rad);
    float view_width_at_object_plane = view_height_at_object_plane * aspect;
    float world_top_y_boundary = gCameraAt.y + view_height_at_object_plane / 2.0f;
    float world_left_x_boundary = gCameraAt.x - view_width_at_object_plane / 2.0f;
    vec3 initialPosition;
    initialPosition.x = world_left_x_boundary + actualVisualRadius;
    initialPosition.y = world_top_y_boundary - actualVisualRadius;
    initialPosition.z = 0.0f;
    return initialPosition;
}

int main() {
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(sceneWidth, sceneHeight, "Sphere Renderer", NULL, NULL); // Cleaned title
    if (!window) { glfwTerminate(); exit(EXIT_FAILURE); }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE; // Should be before glewInit()
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW initialization failed" << std::endl; glfwTerminate(); exit(EXIT_FAILURE); }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    init();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        if (deltaTime > 1.0/20.0) deltaTime = 1.0/20.0; // Cap deltaTime to avoid jumps
        lastTime = currentTime;
        glfwPollEvents();
        Theta[Axis] += rotationSpeed * (float)deltaTime;
        if (Theta[Axis] > 360.0f) Theta[Axis] -= 360.0f;
        else if (Theta[Axis] < 0.0f) Theta[Axis] += 360.0f;
        display();
        glfwSwapBuffers(window);
    }

    // Cleanup
    if (earthTextureID != 0) glDeleteTextures(1, &earthTextureID);
    if (basketballTextureID != 0) glDeleteTextures(1, &basketballTextureID);
    if (synthetic1DTexID != 0) glDeleteTextures(1, &synthetic1DTexID);
    
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereIBO);
    if(program != 0) glDeleteProgram(program);

    std::cout << "OpenGL Process Done!" << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
