#version 330 core
in vec3 viewPos;      // Vertex position in view space (interpolated)
in vec3 viewNormal;   // Vertex normal in view space (interpolated)
in vec4 vertexColor;

out vec4 FragColor;

struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;
    float diffuseIntensity;
};

struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;
uniform DirectionalLight directionalLight;
uniform vec3 eyePosition;
uniform float enableAmbient;
uniform float enableDiffuse;
uniform float enableSpecular;

void main()
{
    // Calculate original components as before
    // Ambient component
    vec3 ambient_calc = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse component
    vec3 normal = normalize(viewNormal);
    vec3 lightDir = normalize(-directionalLight.direction); // Vector from fragment to light.
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse_calc = directionalLight.diffuseIntensity * directionalLight.color * diffuseFactor;
    
    // Specular component
    vec3 specular_calc = vec3(0.0);
    if(diffuseFactor > 0.0f) { // Only calculate specular if light hits the surface
        vec3 fragToEye = normalize(eyePosition - viewPos);
        // Assuming directionalLight.direction is the vector FROM the light source
        vec3 reflectedLightDirection = normalize(reflect(directionalLight.direction, normal));
        float specularFactorVal = dot(fragToEye, reflectedLightDirection); // Renamed to avoid conflict
        if(specularFactorVal > 0.0f) {
            specularFactorVal = pow(specularFactorVal, material.shininess);
            specular_calc = directionalLight.color * material.specularIntensity * specularFactorVal;
        }
    }
    
    // Apply toggles by multiplying
    vec3 finalAmbient = ambient_calc * enableAmbient;
    vec3 finalDiffuse = diffuse_calc * enableDiffuse;
    vec3 finalSpecular = specular_calc * enableSpecular;
    
    // Combine the (potentially toggled off) components
    vec3 result = finalAmbient + finalDiffuse + finalSpecular;
    FragColor = vec4(result * vertexColor.rgb, vertexColor.a);
}
