#version 330 core

// Inputs from Vertex Shader
in vec4 vs_GouraudColor;
in vec3 vs_ViewPos;
in vec3 vs_ViewNormal;
in vec4 vs_VertexColor;
in vec2 vs_TexCoord;    // 2D texture coordinates
in float vs_TexCoordS;  // 1D texture coordinate (s-component)

out vec4 FragColor;

// --- Uniforms (existing and new) ---
uniform int shadingMode;     // 0: Gouraud, 1: Phong
uniform int displayMode;     // From C++: 0 for Shading, 2 for Texture (FS concern)

uniform bool u_isShadowPass; // True if rendering the shadow
uniform vec4 u_shadowColor;  // The color of the shadow

struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;        // Expected in View Space from C++
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;

uniform vec3 eyePosition;    // Expected in View Space
uniform float enableAmbient;
uniform float enableDiffuse;
uniform float enableSpecular;

// --- Texture Samplers and Type ---
uniform sampler2D textureSampler2D; // For 2D textures (earth, basketball)
uniform sampler1D textureSampler1D; // For 1D synthetic texture
uniform int u_currentTextureType;   // 0 for 2D texture, 1 for 1D texture

void main()
{
    if (u_isShadowPass) {
        FragColor = u_shadowColor;
        return;
    }

    // --- Lighting Calculation Components (common for Phong and lit Texture) ---
    vec3 N_eff = normalize(vs_ViewNormal);
    vec3 V_eff = normalize(eyePosition - vs_ViewPos); // eyePosition is in View Space
    vec3 L_to_light_eff_for_diffuse = normalize(-directionalLight.direction); // light.direction is in View Space

    vec3 ambient_light_effect = directionalLight.ambientIntensity * directionalLight.color * enableAmbient;
    float diffFactor = max(dot(N_eff, L_to_light_eff_for_diffuse), 0.0);
    vec3 diffuse_light_effect = directionalLight.diffuseIntensity * directionalLight.color * diffFactor * enableDiffuse;

    vec3 specular_light_effect = vec3(0.0);
    if (diffFactor > 0.0) {
        vec3 I_light_propagation = normalize(directionalLight.direction);
        vec3 R_reflected = reflect(I_light_propagation, N_eff);
        float specFactor = pow(max(dot(V_eff, R_reflected), 0.0), material.shininess);
        specular_light_effect = directionalLight.color * material.specularIntensity * specFactor * enableSpecular;
    }

    // --- Final Color Determination ---
    if (displayMode == 2) { // Texture Mode (can be 1D or 2D)
        vec4 baseTexColor;
        if (u_currentTextureType == 0) { // 2D Texture
            baseTexColor = texture(textureSampler2D, vs_TexCoord);
        } else { // 1D Texture (u_currentTextureType == 1)
            baseTexColor = texture(textureSampler1D, vs_TexCoordS);
        }

        vec3 baseSurfaceColorFromTexture = baseTexColor.rgb;
        float finalAlpha = baseTexColor.a * vs_VertexColor.a;

        vec3 litTextureColor = (ambient_light_effect + diffuse_light_effect) * baseSurfaceColorFromTexture;
        litTextureColor += specular_light_effect;
        FragColor = vec4(litTextureColor, finalAlpha);

    } else { // MODE_SHADING (0) or MODE_WIREFRAME (shader sees 0 for displayMode)
        if (shadingMode == 0) { // Gouraud Shading
            FragColor = vs_GouraudColor;
        } else { // Phong Shading (shadingMode == 1)
            vec3 phong_lit_color = (ambient_light_effect + diffuse_light_effect) * vs_VertexColor.rgb;
            phong_lit_color += specular_light_effect;
            FragColor = vec4(phong_lit_color, vs_VertexColor.a);
        }
    }
}
