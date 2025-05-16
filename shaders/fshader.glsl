#version 330 core
in vec3 worldPos;
in vec3 worldNormal;
in vec4 vertexColor;

out vec4 FragColor;

struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

void main()
{
    // Ambient component
    vec3 ambient = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse component
    vec3 normal = normalize(worldNormal); // Normalize the normal vector
    vec3 lightDir = normalize(-directionalLight.direction); // Light direction (from fragment to light)
    float diff = max(dot(normal, lightDir), 0.0); // Clamp the dot product
    vec3 diffuse = directionalLight.diffuseIntensity * directionalLight.color * diff;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result * vertexColor.rgb, vertexColor.a);
}

