#version 330 core
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec3 vNormal;

uniform mat4 ModelView;
uniform mat4 Projection;
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

// Outputs to Fragment Shader
out vec4 vs_GouraudColor; // For Gouraud result
out vec3 vs_ViewPos;      // For Phong
out vec3 vs_ViewNormal;   // For Phong
out vec4 vs_VertexColor;  // For Phong (and Gouraud base)

void main()
{
    vec4 P_view_h = ModelView * vPosition;

    vs_ViewPos = P_view_h.xyz;
    vs_ViewNormal = normalize(mat3(ModelView) * vNormal);
    vs_VertexColor = vColor;

    if (shadingMode == 0) { // Gouraud Shading Path
        vec3 N = vs_ViewNormal;
        vec3 V = normalize(eyePosition - vs_ViewPos);
        vec3 L_to_light = normalize(-directionalLight.direction);

        vec3 ambient_calc = directionalLight.ambientIntensity * directionalLight.color;
        float diffFactor = max(dot(N, L_to_light), 0.0);
        vec3 diffuse_calc = directionalLight.diffuseIntensity * directionalLight.color * diffFactor;
        vec3 specular_calc = vec3(0.0);
        if (diffFactor > 0.0) {
            vec3 I = normalize(directionalLight.direction);
            vec3 R = reflect(I, N);
            // Using the "Exaggerate difference" test for Gouraud specular
            float gouraud_test_shininess;
            if (material.shininess > 20.0) {
                gouraud_test_shininess = 4.0;
            } else {
                gouraud_test_shininess = 1.0;
            }
            float specFactor = pow(max(dot(V, R), 0.0), gouraud_test_shininess);
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
