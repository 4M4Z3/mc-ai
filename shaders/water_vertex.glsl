#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aAO;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time; // For water animation
uniform vec3 cameraPos; // Camera position for reflection calculations

out float vertexAO;
out vec3 worldPos;
out vec3 viewDir;
out vec3 surfaceNormal;
out vec2 waveCoords;
out float waveHeight;

void main()
{
    // Calculate base world position
    vec4 worldPosition = model * vec4(aPos, 1.0);
    worldPos = worldPosition.xyz;
    
    // Simple, realistic wave generation
    vec2 pos = worldPos.xz * 0.05; // Larger wave patterns for realism
    
    // Just a few gentle waves - much more realistic than complex noise
    float wave1 = sin(pos.x * 1.2 + time * 0.6) * 0.08;  // Large, slow waves
    float wave2 = sin(pos.y * 0.8 + time * 0.4) * 0.06;  // Cross waves
    float wave3 = sin((pos.x + pos.y) * 2.0 + time * 1.0) * 0.03;  // Small detail waves
    
    // Combine waves - keep it simple
    waveHeight = wave1 + wave2 + wave3;
    
    // Only apply wave displacement to top surface (y-component)
    if (aPos.y > 0.5) { // Assuming water blocks have vertices at y=0 (bottom) and y=1 (top)
        worldPos.y += waveHeight * 0.3; // Scale down the displacement for subtlety
    }
    
    // Calculate surface normal from simple wave derivatives
    float dWave_dx = 
        1.2 * 0.05 * cos(pos.x * 1.2 + time * 0.6) * 0.08 +  // wave1 derivative
        2.0 * 0.05 * cos((pos.x + pos.y) * 2.0 + time * 1.0) * 0.03;  // wave3 derivative
    
    float dWave_dz = 
        0.8 * 0.05 * cos(pos.y * 0.8 + time * 0.4) * 0.06 +  // wave2 derivative  
        2.0 * 0.05 * cos((pos.x + pos.y) * 2.0 + time * 1.0) * 0.03;  // wave3 derivative
    
    // Create surface normal from wave gradients
    vec3 tangent = normalize(vec3(1.0, dWave_dx * 0.3, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dWave_dz * 0.3, 1.0));
    surfaceNormal = normalize(cross(tangent, bitangent));
    
    // Calculate view direction from camera to surface point
    viewDir = normalize(cameraPos - worldPos);
    
    // Simple texture coordinates for surface effects
    waveCoords = vec2(worldPos.x, worldPos.z) * 0.1 + vec2(time * 0.05, time * 0.03);
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
    vertexAO = aAO;
}