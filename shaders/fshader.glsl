#version 330 core
// Inputs from Vertex Shader
in vec4 vs_GouraudColor;
in vec3 vs_ViewPos;
in vec3 vs_ViewNormal;
in vec4 vs_VertexColor;

out vec4 FragColor;

uniform int shadingMode;

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

void main()
{
    if (shadingMode == 0) { // Gouraud Shading
        FragColor = vs_GouraudColor; // Use the interpolated color from Vertex Shader
    } else { // Phong Shading (shadingMode == 1)
        vec3 N_phong = normalize(vs_ViewNormal);
        vec3 V_phong = normalize(eyePosition - vs_ViewPos);
        vec3 L_to_light_phong = normalize(-directionalLight.direction);

        vec3 ambient_phong = directionalLight.ambientIntensity * directionalLight.color;
        float diffFactor_phong = max(dot(N_phong, L_to_light_phong), 0.0);
        vec3 diffuse_phong = directionalLight.diffuseIntensity * directionalLight.color * diffFactor_phong;
        vec3 specular_phong = vec3(0.0);
        if (diffFactor_phong > 0.0) {
            vec3 I_phong = normalize(directionalLight.direction);
            vec3 R_phong = reflect(I_phong, N_phong);
            float specFactor_phong = pow(max(dot(V_phong, R_phong), 0.0), material.shininess);
            specular_phong = directionalLight.color * material.specularIntensity * specFactor_phong;
        }

        ambient_phong *= enableAmbient;
        diffuse_phong *= enableDiffuse;
        specular_phong *= enableSpecular;

        vec3 totalLight_phong = ambient_phong + diffuse_phong + specular_phong;
        FragColor = vec4(totalLight_phong * vs_VertexColor.rgb, vs_VertexColor.a);
    }
}
