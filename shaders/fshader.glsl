#version 330 core

in vec3 worldPos;     // Input world-space position
in vec3 worldNormal;  // Input world-space normal
in vec4 vertexColor;  // Keep if needed, otherwise remove
in vec2 fTexCoord;    // Input texture coordinate from vertex shader

out vec4 FragColor;

struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

uniform sampler2D textureSampler; // Add uniform for the texture sampler

void main()
{
    
    // Sample color from the texture
    vec4 textureColor = texture(textureSampler, fTexCoord);

    // Ambient component
    vec3 ambient = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse component
    vec3 normal = normalize(worldNormal); // Normalize the normal vector
    vec3 lightDir = normalize(-directionalLight.direction); // Light direction (from fragment to light)
    float diff = max(dot(normal, lightDir), 0.0); // Clamp the dot product
    vec3 diffuse = directionalLight.diffuseIntensity * directionalLight.color * diff;

    // Combine lighting with the texture color
    // We multiply the lighting result by the texture color.
    // If you have a separate material ambient/diffuse color,
    // you would multiply those with the light components before
    // multiplying by the texture color.
    vec3 lightingResult = ambient + diffuse;
    vec3 finalColor = lightingResult * textureColor.rgb; // Apply lighting to texture color

    FragColor = vec4(finalColor, textureColor.a); // Use texture's alpha
}
