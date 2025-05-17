#version 330 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColor;      // Keep vColor if you still use it for something else, otherwise can remove
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec2 vTexCoord;   // Add texture coordinate input

uniform mat4 ModelView;
uniform mat4 Projection;

out vec3 worldPos;     // Output world-space position
out vec3 worldNormal;  // Output world-space normal
out vec4 vertexColor;  // Keep if needed, otherwise remove
out vec2 fTexCoord;    // Output texture coordinate to fragment shader

void main()
{
    worldPos = (ModelView * vPosition).xyz;     // Transform position to world space
    // When transforming normals, use the inverse transpose of the modelview matrix.
    // For uniform scaling, mat3(ModelView) is sufficient. If non-uniform scaling is used,
    // the inverse transpose is required to correctly transform normals.
    worldNormal = mat3(transpose(inverse(ModelView))) * vNormal;
    vertexColor = vColor;                       // Keep if needed, otherwise remove
    fTexCoord = vTexCoord;                      // Pass texture coordinate to fragment shader
    gl_Position = Projection * ModelView * vPosition;
}
