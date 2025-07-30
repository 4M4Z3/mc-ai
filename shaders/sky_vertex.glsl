#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 viewDir;

void main()
{
    // Remove translation from view matrix to keep sky at infinite distance
    mat4 skyView = mat4(mat3(view)); // Keep only rotation, remove translation
    
    vec4 pos = projection * skyView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Set z to w for infinite distance
    
    // Pass local position as world position for calculations
    worldPos = aPos;
    
    // The view direction is just the vertex position (normalized in fragment shader)
    viewDir = aPos;
} 