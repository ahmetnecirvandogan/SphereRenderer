#version 330 core
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColor;
layout (location = 2) in vec3 vNormal;

uniform mat4 ModelView;
uniform mat4 Projection;

out vec3 worldPos;  // Output world-space position
out vec3 worldNormal; // Output world-space normal
out vec4 vertexColor;

void main()
{
    worldPos = (ModelView * vPosition).xyz; // Transform position to world space
    worldNormal = mat3(ModelView) * vNormal;    // Transform normal to world space.  Use mat3 for normals!
    vertexColor = vColor;
    gl_Position = Projection * ModelView * vPosition;
}

