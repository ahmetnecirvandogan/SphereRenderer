#version 330 core
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec2 aTexCoord; // Existing 2D texture coordinates

uniform mat4 ModelView;
uniform mat4 Projection;
uniform int shadingMode; // 0: Gouraud, 1: Phong

// --- Uniforms for Lighting (existing) ---
struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction; // Expected in View Space from C++
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;

uniform vec3 eyePosition; // Expected in View Space
uniform float enableAmbient;
uniform float enableDiffuse;
uniform float enableSpecular;

// --- NEW Uniforms for 1D Texture Mapping ---
uniform mat4 u_ModelMatrix;           // Object's model matrix (world transformation)
uniform vec4 u_1DTexturePlane;        // Plane for 1D tex coords (Nx, Ny, Nz, D) in World Space
uniform float u_1DTextureStripeScale; // Scale factor for 1D texture stripe frequency
uniform int u_currentTextureType;     // 0 for 2D texture, 1 for 1D texture

// --- Outputs to Fragment Shader ---
out vec4 vs_GouraudColor; // For Gouraud result
out vec3 vs_ViewPos;      // For Phong
out vec3 vs_ViewNormal;   // For Phong
out vec4 vs_VertexColor;  // For Phong (and Gouraud base)
out vec2 vs_TexCoord;     // Pass through 2D tex coords
out float vs_TexCoordS;   // Calculated 1D tex coord (s-component)

void main()
{
    vec4 P_view_h = ModelView * vPosition; // Vertex position in View Space

    vs_ViewPos = P_view_h.xyz;
    vs_ViewNormal = normalize(mat3(ModelView) * vNormal); // Normal in View Space
    vs_VertexColor = vColor;
    vs_TexCoord = aTexCoord; // Pass through 2D texture coordinates

    // Calculate 1D texture coordinate if 1D texture is active
    if (u_currentTextureType == 1) { // 1D Texture is active
        vec4 worldPos = u_ModelMatrix * vPosition; // Vertex position in World Space

        // Calculate signed distance to the plane (assuming u_1DTexturePlane.xyz is normalized)
        // Plane equation: dot(N, P) + D = 0. Distance = dot(N,P) + D
        float distanceToPlane = dot(worldPos.xyz, u_1DTexturePlane.xyz) + u_1DTexturePlane.w;
        vs_TexCoordS = distanceToPlane * u_1DTextureStripeScale;
    } else {
        vs_TexCoordS = 0.0; // Default, not used by 2D texturing path in FS
    }

    // Gouraud Shading Path (existing logic)
    if (shadingMode == 0) {
        vec3 N = vs_ViewNormal;
        vec3 V = normalize(eyePosition - vs_ViewPos); // eyePosition is already in view space
        vec3 L_to_light = normalize(-directionalLight.direction); // light.direction is already in view space

        vec3 ambient_calc = directionalLight.ambientIntensity * directionalLight.color;
        float diffFactor = max(dot(N, L_to_light), 0.0);
        vec3 diffuse_calc = directionalLight.diffuseIntensity * directionalLight.color * diffFactor;
        
        vec3 specular_calc = vec3(0.0);
        if (diffFactor > 0.0) {
            vec3 I_light_propagation = normalize(directionalLight.direction); // Light propagation towards surface
            vec3 R_reflected = reflect(I_light_propagation, N);
            
            float gouraud_test_shininess;
            if (material.shininess > 20.0) { // As per your existing shader
                gouraud_test_shininess = 4.0;
            } else {
                gouraud_test_shininess = 1.0;
            }
            float specFactor = pow(max(dot(V, R_reflected), 0.0), gouraud_test_shininess);
            specular_calc = directionalLight.color * material.specularIntensity * specFactor;
        }

        vec3 finalAmbient = ambient_calc * enableAmbient;
        vec3 finalDiffuse = diffuse_calc * enableDiffuse;
        vec3 finalSpecular = specular_calc * enableSpecular;
        vec3 totalLight = finalAmbient + finalDiffuse + finalSpecular;
        vs_GouraudColor = vec4(totalLight * vs_VertexColor.rgb, vs_VertexColor.a);
    } else { // Phong Shading Path
        vs_GouraudColor = vec4(0.0, 0.0, 0.0, 1.0); // Default for this out, FS Phong path ignores it
    }

    gl_Position = Projection * P_view_h;
}
