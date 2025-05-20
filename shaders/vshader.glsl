#version 330 core
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec3 vNormal;

uniform mat4 ModelView;
uniform mat4 Projection;

out vec3 viewPos;      // Vertex position in view space
out vec3 viewNormal;   // Vertex normal in view space
out vec4 vertexColor;  // Pass-through color

void main()
{
    gl_Position = Projection * ModelView * vPosition;
    vertexColor = vColor;
    viewPos = (ModelView * vPosition).xyz;
    viewNormal = normalize(mat3(ModelView) * vNormal);
}
