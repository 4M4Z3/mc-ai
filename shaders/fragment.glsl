#version 330 core
out vec4 FragColor;

in float vertexAO;

void main()
{
    // Base color (light gray/white)
    vec3 baseColor = vec3(0.9, 0.9, 0.9);
    
    // Apply ambient occlusion (darken based on AO value)
    // AO ranges from 0.0 (fully occluded/dark) to 1.0 (no occlusion/bright)
    vec3 finalColor = baseColor * vertexAO;
    
    FragColor = vec4(finalColor, 1.0);
} 