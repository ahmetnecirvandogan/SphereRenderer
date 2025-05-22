#version 330 core
// Inputs from Vertex Shader
in vec4 vs_GouraudColor;
in vec3 vs_ViewPos;
in vec3 vs_ViewNormal;
in vec4 vs_VertexColor;
in vec2 vs_TexCoord;

out vec4 FragColor;

uniform int shadingMode;     // 0: Gouraud, 1: Phong
uniform int displayMode;     // From C++: 0 for Shading, (1 for Wireframe - FS doesn't care), 2 for Texture
                             // Note: If MODE_SHADING_WITH_SHADOW is 1 in C++, C++ should send 0 to this uniform for the main object.

// Uniforms for shadow pass
uniform bool u_isShadowPass;   // True if rendering the shadow
uniform vec4 u_shadowColor;    // The color of the shadow (e.g., dark grey)

struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;

uniform vec3 eyePosition;
uniform float enableAmbient;
uniform float enableDiffuse;
uniform float enableSpecular;

uniform sampler2D textureSampler; // Your sphere's 2D texture

void main()
{
    if (u_isShadowPass) {
        FragColor = u_shadowColor;
        return;
    }

    // --- Lighting Calculation (common components) ---
    vec3 N_eff = normalize(vs_ViewNormal);
    vec3 V_eff = normalize(eyePosition - vs_ViewPos);
    vec3 L_to_light_eff_for_diffuse = normalize(-directionalLight.direction); // Surface to light
    
    vec3 ambient_light_effect = directionalLight.ambientIntensity * directionalLight.color * enableAmbient;
    float diffFactor = max(dot(N_eff, L_to_light_eff_for_diffuse), 0.0);
    vec3 diffuse_light_effect = directionalLight.diffuseIntensity * directionalLight.color * diffFactor * enableDiffuse;
    
    vec3 specular_light_effect = vec3(0.0);
    if (diffFactor > 0.0) {
        vec3 I_for_reflection = normalize(directionalLight.direction); // Light propagation direction
        vec3 R_reflected = reflect(I_for_reflection, N_eff); // Standard reflect uses incident I
        float specFactor = pow(max(dot(V_eff, R_reflected), 0.0), material.shininess);
        specular_light_effect = directionalLight.color * material.specularIntensity * specFactor * enableSpecular;
    }

    if (displayMode == 2) {
        vec4 texColor = texture(textureSampler, vs_TexCoord); // Use vs_TexCoord from VS
        vec3 baseSurfaceColorFromTexture = texColor.rgb;
        float finalAlpha = texColor.a * vs_VertexColor.a; // Combine alpha

        vec3 litTextureColor = (ambient_light_effect + diffuse_light_effect) * baseSurfaceColorFromTexture;
        litTextureColor += specular_light_effect;
        FragColor = vec4(litTextureColor, finalAlpha);

    } else { // MODE_SHADING or MODE_WIREFRAME (use vs_VertexColor as the base material color)
        
        if (shadingMode == 0) { // Gouraud Shading (no texture override)
            FragColor = vs_GouraudColor; // vs_GouraudColor includes lighting * vs_VertexColor (red)
        } else { // Phong Shading (no texture override)
            vec3 total_light_on_vertex_color = ambient_light_effect + diffuse_light_effect + specular_light_effect;
            FragColor = vec4(total_light_on_vertex_color * vs_VertexColor.rgb, vs_VertexColor.a);
        }
    }
}
