#version 330 core
out vec4 FragColor;

in float vertexAO;
in vec2 TexCoord;

uniform sampler2D blockTexture;

void main()
{
    // Sample the texture
    vec4 textureColor = texture(blockTexture, TexCoord);
    
    // Apply ambient occlusion to the texture color
    // AO values: 1.0=bright, 0.8=light shadow, 0.6=medium shadow, 0.4=dark shadow, 0.25=very dark
    vec3 finalColor = textureColor.rgb * vertexAO;
    
    FragColor = vec4(finalColor, 1.0);
} 