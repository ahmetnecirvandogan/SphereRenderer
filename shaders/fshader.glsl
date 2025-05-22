#version 330 core
// Inputs from Vertex Shader
in vec4 vs_GouraudColor;
in vec3 vs_ViewPos;
in vec3 vs_ViewNormal;
in vec4 vs_VertexColor; // This is your (1.0, 0.0, 0.0, 1.0) color
in vec2 vs_TexCoord;    // Texture coordinates

out vec4 FragColor;

uniform int shadingMode;     // 0: Gouraud, 1: Phong
uniform int displayMode;     // 0: Shading, 1: Wireframe, 2: Texture

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

uniform bool u_isShadowPass;   // New uniform
uniform vec4 u_shadowColor;    // New uniform (e.g., (0.2, 0.2, 0.2, 0.5))

uniform sampler2D textureSampler; // Your sphere's texture

void main()
{
    if (u_isShadowPass) {
        FragColor = u_shadowColor;
        return; // Skip all other calculations for shadow
    }
    
    // --- Lighting Calculation (common components) ---
    // These components represent the light's interaction, independent of base surface color initially
    vec3 N_eff = normalize(vs_ViewNormal);
    vec3 V_eff = normalize(eyePosition - vs_ViewPos);
    // Assuming directionalLight.direction is the light's propagation vector (from the light source)
    vec3 L_incident_norm = normalize(directionalLight.direction); // Vector from light source, incident on surface
    vec3 L_surface_to_light_norm = -L_incident_norm; // Vector from surface point towards light source

    vec3 ambient_light_effect = directionalLight.ambientIntensity * directionalLight.color * enableAmbient;
    
    float diffFactor = max(dot(N_eff, L_incident_norm), 0.0); // Or dot(N_eff, L_surface_to_light_norm) if L_surface_to_light_norm was normalize(-directionalLight.direction)
                                                               // Your original Phong used L_to_light_phong = normalize(-directionalLight.direction)
                                                               // and then dot(N_phong, L_to_light_phong). So this is consistent.
                                                               // Let's stick to your original variable naming for diffuse:
    vec3 L_to_light_eff_for_diffuse = normalize(-directionalLight.direction);
    diffFactor = max(dot(N_eff, L_to_light_eff_for_diffuse), 0.0);
    vec3 diffuse_light_effect = directionalLight.diffuseIntensity * directionalLight.color * diffFactor * enableDiffuse;
    
    vec3 specular_light_effect = vec3(0.0);
    if (diffFactor > 0.0) {
        vec3 I_for_reflection = normalize(directionalLight.direction); // Incident vector for reflect()
        vec3 R_reflected = reflect(I_for_reflection, N_eff);
        float specFactor = pow(max(dot(V_eff, R_reflected), 0.0), material.shininess);
        specular_light_effect = directionalLight.color * material.specularIntensity * specFactor * enableSpecular;
    }

    // --- Determine Final Color based on Display Mode and Shading Mode ---

    if (displayMode == 2 /* MODE_TEXTURE */) {
        vec4 texColor = texture(textureSampler, vs_TexCoord);
        vec3 baseSurfaceColorFromTexture = texColor.rgb;
        float finalAlpha = texColor.a * vs_VertexColor.a; // Combine alpha

        // For Texture Mode, always use per-pixel lighting calculations for better quality with texture
        // Apply lighting effects to the texture color
        vec3 litTextureColor = (ambient_light_effect + diffuse_light_effect) * baseSurfaceColorFromTexture;
        litTextureColor += specular_light_effect; // Specular highlights are typically added

        FragColor = vec4(litTextureColor, finalAlpha);

    } else { // MODE_SHADING or MODE_WIREFRAME (use vs_VertexColor as the base material color)
        
        if (shadingMode == 0) { // Gouraud Shading (no texture override)
            // vs_GouraudColor already contains the full lit color including vs_VertexColor (red)
            FragColor = vs_GouraudColor;
        } else { // Phong Shading (no texture override)
            // Apply lighting effects to the vs_VertexColor (red)
            vec3 total_light_on_vertex_color = ambient_light_effect + diffuse_light_effect + specular_light_effect;
            FragColor = vec4(total_light_on_vertex_color * vs_VertexColor.rgb, vs_VertexColor.a);
        }
    }
}
