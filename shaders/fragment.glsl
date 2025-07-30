#version 330 core
out vec4 FragColor;

in float vertexAO;
in vec2 TexCoord;

uniform sampler2D blockTexture;
uniform vec3 colorTint; // RGB color multiplier for tinting
uniform float cameraY; // Camera Y position for underwater effect

void main()
{
    // Sample the texture
    vec4 textureColor = texture(blockTexture, TexCoord);
    
    // Discard transparent pixels (for overlay textures)
    if (textureColor.a < 0.1) {
        discard;
    }
    
    // Apply color tint to the texture
    vec3 tintedColor = textureColor.rgb * colorTint;
    
    // Apply ambient occlusion to the tinted color
    // AO values: 1.0=bright, 0.8=light shadow, 0.6=medium shadow, 0.4=dark shadow, 0.25=very dark
    vec3 finalColor = tintedColor * vertexAO;
    
    // Apply underwater blue tint if camera is below water level (y < 60)
    if (cameraY < 60.0) {
        // Mix with blue tint for underwater effect
        vec3 underwaterTint = vec3(0.4, 0.7, 1.0); // Blue tint
        float tintStrength = 0.4; // How much blue tint to apply
        finalColor = mix(finalColor, finalColor * underwaterTint, tintStrength);
    }
    
    FragColor = vec4(finalColor, 1.0);
} 