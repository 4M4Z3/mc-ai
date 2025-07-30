#include "BiomeSystem.h"
#include <cmath>

// Biome color data based on your specifications
// Grass Colors: Plains: #91BD59, Forest: #79C05A, Taiga: #86B783, Swamp: #6A7039, 
//               Jungle: #59C93C, Desert: #BFB755, Savanna: #BFB755, Snowy: #80B497,
//               Mushroom: #55C93F, Badlands: #90814D
// Foliage Colors: Plains: #77AB2F, Forest: #59AE30, Taiga: #68A464, Swamp: #6A7039,
//                 Jungle: #30BB0B, Desert: #AEA42A, Savanna: #AEA42A, Snowy: #60A17B,
//                 Mushroom: #2BBB0F, Badlands: #9E814D
const std::array<BiomeColors, static_cast<int>(BiomeType::COUNT)> BiomeSystem::s_biomeColors = {{
    // PLAINS
    {0.568f, 0.741f, 0.349f,  0.467f, 0.671f, 0.184f},
    // FOREST
    {0.475f, 0.753f, 0.353f,  0.349f, 0.682f, 0.188f},
    // TAIGA
    {0.525f, 0.718f, 0.514f,  0.408f, 0.643f, 0.392f},
    // SWAMP
    {0.416f, 0.439f, 0.224f,  0.416f, 0.439f, 0.224f},
    // JUNGLE
    {0.349f, 0.788f, 0.235f,  0.188f, 0.733f, 0.043f},
    // DESERT
    {0.749f, 0.718f, 0.333f,  0.682f, 0.643f, 0.165f},
    // SAVANNA
    {0.749f, 0.718f, 0.333f,  0.682f, 0.643f, 0.165f},
    // SNOWY
    {0.502f, 0.706f, 0.592f,  0.376f, 0.631f, 0.482f},
    // MUSHROOM_ISLAND
    {0.333f, 0.788f, 0.247f,  0.169f, 0.733f, 0.059f},
    // BADLANDS
    {0.565f, 0.506f, 0.302f,  0.620f, 0.506f, 0.302f}
}};

BiomeType BiomeSystem::GetBiomeType(int worldX, int worldZ, int seed) {
    // Use Perlin noise to determine biome (simplified version)
    // This is a basic implementation - you could make it more sophisticated
    double noise1 = sin(worldX * 0.01 + seed) * cos(worldZ * 0.01 + seed);
    double noise2 = sin(worldX * 0.005 + seed * 2) * cos(worldZ * 0.005 + seed * 2);
    double combinedNoise = (noise1 + noise2) * 0.5;
    
    // Map noise to biome index
    double normalizedNoise = (combinedNoise + 1.0) * 0.5; // Map from [-1,1] to [0,1]
    int biomeIndex = static_cast<int>(normalizedNoise * static_cast<int>(BiomeType::COUNT));
    biomeIndex = std::max(0, std::min(biomeIndex, static_cast<int>(BiomeType::COUNT) - 1));
    
    return static_cast<BiomeType>(biomeIndex);
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