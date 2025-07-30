#include "BiomeSystem.h"
#include <cmath>
#include <algorithm>

// Biome color data based on your specifications
// Updated for new biome system
const std::array<BiomeColors, static_cast<int>(BiomeType::COUNT)> BiomeSystem::s_biomeColors = {{
    // SNOWY_TUNDRA
    {0.502f, 0.706f, 0.592f,  0.376f, 0.631f, 0.482f},
    // TAIGA 
    {0.525f, 0.718f, 0.514f,  0.408f, 0.643f, 0.392f},
    // SNOWY_TAIGA
    {0.502f, 0.706f, 0.592f,  0.376f, 0.631f, 0.482f},
    // FOREST
    {0.475f, 0.753f, 0.353f,  0.349f, 0.682f, 0.188f},
    // PLAINS
    {0.568f, 0.741f, 0.349f,  0.467f, 0.671f, 0.184f},
    // SWAMP
    {0.416f, 0.439f, 0.224f,  0.416f, 0.439f, 0.224f},
    // JUNGLE
    {0.349f, 0.788f, 0.235f,  0.188f, 0.733f, 0.043f},
    // SAVANNA
    {0.749f, 0.718f, 0.333f,  0.682f, 0.643f, 0.165f},
    // DESERT
    {0.749f, 0.718f, 0.333f,  0.682f, 0.643f, 0.165f},
    // MUSHROOM_ISLAND
    {0.333f, 0.788f, 0.247f,  0.169f, 0.733f, 0.059f},
    // BADLANDS
    {0.565f, 0.506f, 0.302f,  0.620f, 0.506f, 0.302f},
    // RIVER
    {0.568f, 0.741f, 0.349f,  0.467f, 0.671f, 0.184f}  // Same as plains for river banks
}};

BiomeType BiomeSystem::GetBiomeType(int worldX, int worldZ, int seed) {
    // Check for rivers first (they override other biomes)
    if (IsRiver(worldX, worldZ, seed)) {
        return BiomeType::RIVER;
    }
    
    // Get temperature and humidity for this location
    TemperatureHumidity climate = GetTemperatureHumidity(worldX, worldZ, seed);
    
    // Determine biome from climate
    return GetBiomeFromClimate(climate.temperature, climate.humidity);
}

TemperatureHumidity BiomeSystem::GetTemperatureHumidity(int worldX, int worldZ, int seed) {
    // Use multiple octaves of noise for more natural climate patterns
    double tempNoise1 = PerlinNoise(worldX, worldZ, seed, 0.005);        // Large scale
    double tempNoise2 = PerlinNoise(worldX, worldZ, seed + 1000, 0.02);  // Medium scale
    double tempNoise3 = PerlinNoise(worldX, worldZ, seed + 2000, 0.08);  // Small scale
    
    double humidNoise1 = PerlinNoise(worldX, worldZ, seed + 3000, 0.006);   // Large scale
    double humidNoise2 = PerlinNoise(worldX, worldZ, seed + 4000, 0.025);   // Medium scale
    double humidNoise3 = PerlinNoise(worldX, worldZ, seed + 5000, 0.09);    // Small scale
    
    // Combine noise octaves with different weights
    double temperature = (tempNoise1 * 0.6 + tempNoise2 * 0.3 + tempNoise3 * 0.1);
    double humidity = (humidNoise1 * 0.6 + humidNoise2 * 0.3 + humidNoise3 * 0.1);
    
    // Add some correlation between temperature and humidity (hot areas can be dry)
    humidity -= temperature * 0.2;
    
    // Normalize to [0, 1] range
    temperature = std::max(0.0, std::min(1.0, (temperature + 1.0) * 0.5));
    humidity = std::max(0.0, std::min(1.0, (humidity + 1.0) * 0.5));
    
    return {static_cast<float>(temperature), static_cast<float>(humidity)};
}

BiomeType BiomeSystem::GetBiomeFromClimate(float temperature, float humidity) {
    // Minecraft-like biome determination based on temperature and humidity
    
    // Cold biomes (temperature < 0.3)
    if (temperature < 0.3f) {
        if (humidity < 0.4f) {
            return BiomeType::SNOWY_TUNDRA;
        } else if (humidity < 0.7f) {
            return BiomeType::TAIGA;
        } else {
            return BiomeType::SNOWY_TAIGA;
        }
    }
    // Temperate biomes (0.3 <= temperature < 0.7)
    else if (temperature < 0.7f) {
        if (humidity < 0.3f) {
            return BiomeType::PLAINS;
        } else if (humidity < 0.7f) {
            return BiomeType::FOREST;
        } else {
            return BiomeType::SWAMP;
        }
    }
    // Hot biomes (temperature >= 0.7)
    else {
        if (humidity < 0.3f) {
            return BiomeType::DESERT;
        } else if (humidity < 0.6f) {
            return BiomeType::SAVANNA;
        } else {
            return BiomeType::JUNGLE;
        }
    }
}

bool BiomeSystem::IsRiver(int worldX, int worldZ, int seed) {
    // Create rivers at biome boundaries using ridge noise
    double riverNoise1 = PerlinNoise(worldX, worldZ, seed + 6000, 0.008);
    double riverNoise2 = PerlinNoise(worldX, worldZ, seed + 7000, 0.004);
    
    // Create ridges (rivers flow in valleys)
    double ridge1 = 1.0 - std::abs(riverNoise1);
    double ridge2 = 1.0 - std::abs(riverNoise2);
    
    // Combine ridges
    double riverValue = ridge1 * ridge2;
    
    // Only create rivers where the value is high enough
    // This creates narrow river channels
    return riverValue > 0.85;
}

double BiomeSystem::PerlinNoise(double x, double z, int seed, double scale) {
    // Simple Perlin-like noise implementation
    x *= scale;
    z *= scale;
    
    // Add seed offset
    x += seed * 0.1;
    z += seed * 0.1;
    
    // Integer coordinates
    int xi = static_cast<int>(std::floor(x)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;
    
    // Fractional coordinates
    double xf = x - std::floor(x);
    double zf = z - std::floor(z);
    
    // Smoothing function
    auto fade = [](double t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    
    double u = fade(xf);
    double v = fade(zf);
    
    // Simple hash function for gradients
    auto hash = [seed](int x, int z) {
        int h = seed + x * 374761393 + z * 668265263;
        h = (h ^ (h >> 13)) * 1274126177;
        return (h ^ (h >> 16)) & 3;
    };
    
    // Get gradient vectors
    auto grad = [](int hash, double x, double z) {
        switch(hash) {
            case 0: return x + z;
            case 1: return -x + z;
            case 2: return x - z;
            case 3: return -x - z;
            default: return 0.0;
        }
    };
    
    // Calculate gradients at corners
    double g1 = grad(hash(xi, zi), xf, zf);
    double g2 = grad(hash(xi + 1, zi), xf - 1, zf);
    double g3 = grad(hash(xi, zi + 1), xf, zf - 1);
    double g4 = grad(hash(xi + 1, zi + 1), xf - 1, zf - 1);
    
    // Interpolate
    double lerp1 = g1 + u * (g2 - g1);
    double lerp2 = g3 + u * (g4 - g3);
    
    return lerp1 + v * (lerp2 - lerp1);
}

void BiomeSystem::GetGrassColor(BiomeType biome, float& r, float& g, float& b) {
    const BiomeColors& colors = s_biomeColors[static_cast<int>(biome)];
    r = colors.grassR;
    g = colors.grassG;
    b = colors.grassB;
}

void BiomeSystem::GetFoliageColor(BiomeType biome, float& r, float& g, float& b) {
    const BiomeColors& colors = s_biomeColors[static_cast<int>(biome)];
    r = colors.foliageR;
    g = colors.foliageG;
    b = colors.foliageB;
}

void BiomeSystem::HexToRGB(int hexColor, float& r, float& g, float& b) {
    r = ((hexColor >> 16) & 0xFF) / 255.0f;
    g = ((hexColor >> 8) & 0xFF) / 255.0f;
    b = (hexColor & 0xFF) / 255.0f;
}