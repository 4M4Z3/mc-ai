#version 330 core
out vec4 FragColor;

in float vertexAO;

void main()
{
    // Base color - closer to Minecraft's block color
    vec3 baseColor = vec3(0.95, 0.95, 0.95);
    
    // Apply ambient occlusion with Minecraft-style shading
    // AO values: 1.0=bright, 0.8=light shadow, 0.6=medium shadow, 0.4=dark shadow, 0.25=very dark
    vec3 finalColor = baseColor * vertexAO;
    
    FragColor = vec4(finalColor, 1.0);
} 