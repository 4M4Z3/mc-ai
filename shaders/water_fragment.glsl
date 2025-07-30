#version 330 core
out vec4 FragColor;

in float vertexAO;
in vec3 worldPos;
in vec3 viewDir;
in vec3 surfaceNormal;
in vec2 waveCoords;
in float waveHeight;

uniform float time; // For water animation
uniform float gameTime; // Game time for sky color synchronization
uniform vec3 cameraPos; // Camera position
uniform vec3 sunDirection; // Sun direction for specular highlights

// Function to calculate sky color based on viewing direction and game time
// This mirrors the sky shader's color calculation system
vec3 calculateSkyColor(vec3 direction, float gameTime) {
    float height = normalize(direction).y;
    
    // Get position within current 15-minute cycle (0-900 seconds)
    float cycleTime = mod(gameTime, 900.0);
    float normalizedTime = cycleTime / 900.0;
    
    // Define key times for smooth transitions
    float dawn = 0.0;
    float sunrise = 0.167;
    float morning = 0.333;
    float noon = 0.5;
    float afternoon = 0.667;
    float sunset = 0.833;
    float dusk = 1.0;
    
    vec3 skyColor;
    vec3 horizonColor;
    vec3 groundColor;
    
    if (normalizedTime < sunrise) {
        // Dawn to sunrise
        float t = smoothstep(0.0, 1.0, normalizedTime / sunrise);
        skyColor = mix(vec3(0.2, 0.3, 0.6), vec3(0.8, 0.6, 0.4), t);
        horizonColor = mix(vec3(0.4, 0.3, 0.5), vec3(1.0, 0.5, 0.2), t);
        groundColor = mix(vec3(0.15, 0.12, 0.18), vec3(0.4, 0.2, 0.1), t);
    } else if (normalizedTime < morning) {
        // Sunrise to morning
        float t = smoothstep(0.0, 1.0, (normalizedTime - sunrise) / (morning - sunrise));
        skyColor = mix(vec3(0.8, 0.6, 0.4), vec3(0.53, 0.81, 0.98), t);
        horizonColor = mix(vec3(1.0, 0.5, 0.2), vec3(0.8, 0.9, 1.0), t);
        groundColor = mix(vec3(0.4, 0.2, 0.1), vec3(0.45, 0.42, 0.38), t);
    } else if (normalizedTime < sunset) {
        // Day time (morning to sunset)
        float t = smoothstep(0.0, 1.0, (normalizedTime - morning) / (sunset - morning));
        skyColor = mix(vec3(0.53, 0.81, 0.98), vec3(0.9, 0.7, 0.5), t);
        horizonColor = mix(vec3(0.8, 0.9, 1.0), vec3(1.0, 0.6, 0.3), t);
        groundColor = mix(vec3(0.45, 0.42, 0.38), vec3(0.5, 0.3, 0.15), t);
    } else {
        // Sunset to dusk
        float t = smoothstep(0.0, 1.0, (normalizedTime - sunset) / (dusk - sunset));
        skyColor = mix(vec3(0.9, 0.7, 0.5), vec3(0.2, 0.3, 0.6), t);
        horizonColor = mix(vec3(1.0, 0.6, 0.3), vec3(0.4, 0.3, 0.5), t);
        groundColor = mix(vec3(0.5, 0.3, 0.15), vec3(0.15, 0.12, 0.18), t);
    }
    
    vec3 finalColor;
    if (height > 0.0) {
        // Looking above horizon
        float skyFactor = smoothstep(0.0, 0.8, height);
        skyFactor = pow(skyFactor, 0.6);
        finalColor = mix(horizonColor, skyColor, skyFactor);
        
        // Add atmospheric haze near horizon
        float hazeEffect = 1.0 - smoothstep(0.0, 0.3, height);
        vec3 hazeColor = mix(horizonColor, vec3(0.9, 0.95, 1.0), 0.2);
        finalColor = mix(finalColor, hazeColor, hazeEffect * 0.15);
    } else {
        // Looking below horizon
        float groundFactor = smoothstep(0.0, 0.9, -height);
        groundFactor = pow(groundFactor, 0.7);
        finalColor = mix(horizonColor, groundColor, groundFactor);
    }
    
    return max(finalColor, vec3(0.015, 0.015, 0.025));
}

void main()
{
    // Clean, realistic water colors
    vec3 clearWaterColor = vec3(0.05, 0.3, 0.4);
    vec3 deepWaterColor = vec3(0.02, 0.15, 0.25);
    
    // Calculate view direction and surface normal
    vec3 normal = normalize(surfaceNormal);
    vec3 viewDirection = normalize(viewDir);
    vec3 reflectionDir = reflect(-viewDirection, normal);
    
    // Get sky color for reflections
    vec3 reflectionColor = calculateSkyColor(reflectionDir, gameTime);
    
    // Proper Fresnel effect - more transparent looking down, less transparent at grazing angles
    float NdotV = max(dot(normal, viewDirection), 0.0);
    float fresnel = pow(1.0 - NdotV, 2.0);  // Stronger transparency when looking straight down
    fresnel = mix(0.15, 0.85, fresnel);  // More transparent range
    
    // Distance-based blending (keep this from before since user liked it)
    float distanceToWater = length(cameraPos - worldPos);
    float distanceFactor = smoothstep(8.0, 30.0, distanceToWater);
    distanceFactor = clamp(distanceFactor, 0.0, 0.7);
    
    // Depth-based color
    float depth = max(0.0, 64.0 - worldPos.y) / 15.0;
    depth = clamp(depth, 0.0, 1.0);
    vec3 baseWaterColor = mix(clearWaterColor, deepWaterColor, depth * 0.6);
    
    // Simple specular highlights - just occasional sparkles on wave peaks
    vec3 localSunDir = normalize(sunDirection);
    vec3 halfVector = normalize(viewDirection + localSunDir);
    float specular = pow(max(dot(normal, halfVector), 0.0), 128.0);  // Sharp highlights
    specular *= max(dot(normal, localSunDir), 0.0);
    
    // Only show specular on wave peaks for those occasional sparkles
    float waveSparkle = smoothstep(0.05, 0.12, abs(waveHeight)) * specular;
    
    // Combine reflection with water color
    float finalReflectionFactor = mix(fresnel * 0.2, fresnel * 0.6, distanceFactor);
    vec3 finalColor = mix(baseWaterColor, reflectionColor, finalReflectionFactor);
    
    // Add the occasional sparkles
    finalColor += vec3(waveSparkle) * 0.6;
    
    // Angle-based transparency - key feature requested
    // More transparent looking down (high NdotV), less transparent at grazing angles (low NdotV)
    float alpha = mix(0.3, 0.85, fresnel);  // 0.3 = very transparent looking down, 0.85 = less transparent at angles
    alpha = mix(alpha, 0.6, depth * 0.3);  // Slightly more opaque in deeper water
    
    // Apply ambient occlusion very subtly
    finalColor *= (0.9 + vertexAO * 0.1);
    
    FragColor = vec4(finalColor, alpha);
}