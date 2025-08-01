#version 330 core
out vec4 FragColor;

in float vertexAO;
in vec2 TexCoord;

uniform sampler2D blockTexture;
uniform vec3 colorTint; // RGB color multiplier for tinting
uniform float enableAO; // 1.0 = enable AO, 0.0 = disable AO (pure white)
uniform float enableTexture; // 1.0 = enable texture, 0.0 = use white color
uniform float fadeFactor; // 0.0-1.0 for smooth transitions between modes

void main()
{
    // Always sample the texture for proper transitions
    vec4 texColor = texture(blockTexture, TexCoord);
    if (texColor.a < 0.1) {
        discard;
    }
    
    // Calculate base colors for different modes
    vec3 whiteColor = vec3(1.0, 1.0, 1.0);
    
    // AO-only color (white with ambient occlusion - grayscale)
    vec3 aoOnlyColor = whiteColor * vertexAO;
    
    // Full textured color with AO
    vec3 texturedColor = texColor.rgb * colorTint * vertexAO;
    
    // Determine which transition we're in based on enableAO and enableTexture
    vec3 finalColor;
    
    if (enableTexture < 0.5 && enableAO < 0.5) {
        // WHITE_ONLY mode - could be transitioning to AO_ONLY
        finalColor = mix(whiteColor, aoOnlyColor, fadeFactor);
    } else if (enableTexture < 0.5 && enableAO > 0.5) {
        // AO_ONLY mode - could be transitioning to FULL_RENDER  
        // Fade from grayscale AO to full color textured AO
        finalColor = mix(aoOnlyColor, texturedColor, fadeFactor);
    } else {
        // FULL_RENDER mode - full textures with AO
        finalColor = texturedColor;
    }
    
    FragColor = vec4(finalColor, 1.0);
} 