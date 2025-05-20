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

uniform DirectionalLight directionalLight;
uniform Material material;

uniform vec3 eyePosition;

void main()
{
    // Ambient component
    vec3 ambient = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse component
    vec3 normal = normalize(viewNormal);
    // Light direction is from fragment to light.
    vec3 lightDir = normalize(-directionalLight.direction);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = directionalLight.diffuseIntensity * directionalLight.color * diffuseFactor;
    
    // Specular component
    vec3 specularColor = vec3(0.0); // Initialize to black
    
    if(diffuseFactor > 0.0f) {
        // Vector from fragment to eye (in view space, eye is at origin)
        vec3 fragToEye = normalize(eyePosition - viewPos); // eyePosition is (0,0,0) in view space

        // Reflection vector
        vec3 reflectedLightDirection = normalize(reflect(directionalLight.direction, normal));
        
        float specularFactor = dot(fragToEye, reflectedLightDirection);
        if(specularFactor > 0.0f) {
            specularFactor = pow(specularFactor, material.shininess);
            specularColor = directionalLight.color * material.specularIntensity * specularFactor;
        }
    }
    
    vec3 result = ambient + diffuse + specularColor;
    FragColor = vec4(result * vertexColor.rgb, vertexColor.a);
}
