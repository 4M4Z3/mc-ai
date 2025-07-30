#version 330 core
out vec4 FragColor;

in vec3 worldPos;
in vec3 viewDir;

uniform float gameTime; // Time of day for color transitions
uniform vec3 sunDirection; // Direction to sun for atmospheric scattering

void main()
{
    // Calculate height factor (-1.0 looking down, 0.0 at horizon, 1.0 looking up)
    float height = normalize(viewDir).y;
    
    // Calculate time factor (0.0 = start of day, 1.0 = end of day cycle)
    float timeFactor = gameTime / 900.0f; // 900 seconds = 15 minutes
    
    vec3 skyColor;
    vec3 horizonColor;
    vec3 groundColor; // Color for looking downward
    
    // Get position within current 15-minute cycle (0-900 seconds)
    float cycleTime = mod(gameTime, 900.0);
    // Normalize to 0-1 for color transitions
    float normalizedTime = cycleTime / 900.0;
    
    // Define key times for smooth transitions over 15 minutes
    float dawn = 0.0;        // 0 minutes - start (dawn)
    float sunrise = 0.167;   // 2.5 minutes - sunrise
    float morning = 0.333;   // 5 minutes - morning
    float noon = 0.5;        // 7.5 minutes - noon
    float afternoon = 0.667; // 10 minutes - afternoon
    float sunset = 0.833;    // 12.5 minutes - sunset
    float dusk = 1.0;        // 15 minutes - end (dusk)
    
    if (normalizedTime < sunrise) {
        // Dawn to sunrise (0 to 2.5 minutes)
        float t = normalizedTime / sunrise;
        t = smoothstep(0.0, 1.0, t);
        
        skyColor = mix(
            vec3(0.2, 0.3, 0.6),         // Dawn blue
            vec3(0.8, 0.6, 0.4)          // Sunrise warm
        , t);
        horizonColor = mix(
            vec3(0.4, 0.3, 0.5),         // Dawn purple horizon
            vec3(1.0, 0.5, 0.2)          // Sunrise orange
        , t);
        groundColor = mix(
            vec3(0.15, 0.12, 0.18),      // Dawn ground
            vec3(0.4, 0.2, 0.1)          // Sunrise ground
        , t);
        
    } else if (normalizedTime < morning) {
        // Sunrise to morning (2.5 to 5 minutes)
        float t = (normalizedTime - sunrise) / (morning - sunrise);
        t = smoothstep(0.0, 1.0, t);
        
        skyColor = mix(
            vec3(0.8, 0.6, 0.4),         // Sunrise warm
            vec3(0.4, 0.7, 0.9)          // Morning blue
        , t);
        horizonColor = mix(
            vec3(1.0, 0.5, 0.2),         // Sunrise orange
            vec3(0.7, 0.8, 0.95)         // Morning light
        , t);
        groundColor = mix(
            vec3(0.4, 0.2, 0.1),         // Sunrise ground
            vec3(0.4, 0.35, 0.3)         // Morning ground
        , t);
        
    } else if (normalizedTime < afternoon) {
        // Morning to noon (5 to 10 minutes)
        float t = (normalizedTime - morning) / (afternoon - morning);
        t = smoothstep(0.0, 1.0, t);
        
        skyColor = mix(
            vec3(0.4, 0.7, 0.9),         // Morning blue
            vec3(0.53, 0.81, 0.98)       // Bright noon blue
        , t);
        horizonColor = mix(
            vec3(0.7, 0.8, 0.95),        // Morning light
            vec3(0.8, 0.9, 1.0)          // Bright noon horizon
        , t);
        groundColor = mix(
            vec3(0.4, 0.35, 0.3),        // Morning ground
            vec3(0.45, 0.42, 0.38)       // Day ground
        , t);
        
    } else if (normalizedTime < sunset) {
        // Afternoon to sunset (10 to 12.5 minutes)
        float t = (normalizedTime - afternoon) / (sunset - afternoon);
        t = smoothstep(0.0, 1.0, t);
        
        skyColor = mix(
            vec3(0.53, 0.81, 0.98),      // Bright day blue
            vec3(0.9, 0.7, 0.5)          // Golden afternoon
        , t);
        horizonColor = mix(
            vec3(0.8, 0.9, 1.0),         // Bright horizon
            vec3(1.0, 0.6, 0.3)          // Golden sunset
        , t);
        groundColor = mix(
            vec3(0.45, 0.42, 0.38),      // Day ground
            vec3(0.5, 0.3, 0.15)         // Sunset ground
        , t);
        
    } else {
        // Sunset to dusk (12.5 to 15 minutes)
        float t = (normalizedTime - sunset) / (dusk - sunset);
        t = smoothstep(0.0, 1.0, t);
        
        skyColor = mix(
            vec3(0.9, 0.7, 0.5),         // Golden sunset
            vec3(0.2, 0.3, 0.6)          // Dusk blue
        , t);
        horizonColor = mix(
            vec3(1.0, 0.6, 0.3),         // Golden sunset
            vec3(0.4, 0.3, 0.5)          // Dusk purple
        , t);
        groundColor = mix(
            vec3(0.5, 0.3, 0.15),        // Sunset ground
            vec3(0.15, 0.12, 0.18)       // Dusk ground
        , t);
    }
    
    // Smooth blending from ground to horizon to sky (no hard divide)
    // height ranges from -1.0 (looking down) to 1.0 (looking up)
    
    vec3 finalColor;
    
    if (height > 0.0) {
        // Looking above horizon - blend from horizon to sky
        float skyFactor = smoothstep(0.0, 0.8, height);
        skyFactor = pow(skyFactor, 0.6); // Softer curve
        finalColor = mix(horizonColor, skyColor, skyFactor);
        
        // Add atmospheric haze near horizon
        float hazeEffect = 1.0 - smoothstep(0.0, 0.3, height);
        vec3 hazeColor = mix(horizonColor, vec3(0.9, 0.95, 1.0), 0.2);
        finalColor = mix(finalColor, hazeColor, hazeEffect * 0.15);
        
    } else {
        // Looking below horizon - blend from horizon to ground
        float groundFactor = smoothstep(0.0, 0.9, -height);
        groundFactor = pow(groundFactor, 0.7);
        finalColor = mix(horizonColor, groundColor, groundFactor);
    }
    
    // Add smooth transition across the horizon line
    float crossHorizonBlend = smoothstep(-0.1, 0.1, height);
    if (height > -0.1 && height < 0.1) {
        // Very close to horizon - ensure smooth blending
        vec3 aboveHorizon = mix(horizonColor, skyColor, smoothstep(0.0, 0.8, max(0.0, height)));
        vec3 belowHorizon = mix(horizonColor, groundColor, smoothstep(0.0, 0.9, max(0.0, -height)));
        finalColor = mix(belowHorizon, aboveHorizon, crossHorizonBlend);
    }
    
    // Enhanced atmospheric effects based on time of day
    float atmosphericIntensity = 1.0;
    if (normalizedTime > sunrise && normalizedTime < morning) {
        // Sunrise atmospheric glow
        float glowFactor = (normalizedTime - sunrise) / (morning - sunrise);
        atmosphericIntensity = 1.0 + 0.3 * sin(glowFactor * 3.14159);
    } else if (normalizedTime > afternoon && normalizedTime < dusk) {
        // Sunset atmospheric glow  
        float glowFactor = (normalizedTime - afternoon) / (dusk - afternoon);
        atmosphericIntensity = 1.0 + 0.3 * sin(glowFactor * 3.14159);
    }
    
    // Apply atmospheric effects
    float atmosphericFade = 1.0 - (0.03 * abs(height));
    finalColor *= (0.85 + 0.15 * atmosphericFade) * atmosphericIntensity;
    
    // Add subtle color variations for more dynamic sky
    float noise = sin(viewDir.x * 6.0 + gameTime * 0.01) * cos(viewDir.z * 6.0 + gameTime * 0.01) * 0.008;
    finalColor += noise;
    
    // Add time-based color temperature shifts
    if (normalizedTime < morning || normalizedTime > afternoon) {
        // Warmer tones during sunrise/sunset periods
        finalColor.r *= 1.05;
        finalColor.g *= 1.02;
    } else if (normalizedTime > morning && normalizedTime < afternoon) {
        // Cooler tones during day
        finalColor.b *= 1.03;
    }
    
    // Ensure minimum brightness
    finalColor = max(finalColor, vec3(0.015, 0.015, 0.025));
    
    FragColor = vec4(finalColor, 1.0);
} 