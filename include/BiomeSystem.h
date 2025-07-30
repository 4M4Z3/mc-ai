#pragma once

#include <array>

enum class BiomeType : int {
    PLAINS = 0,
    FOREST = 1,
    TAIGA = 2,
    SWAMP = 3,
    JUNGLE = 4,
    DESERT = 5,
    SAVANNA = 6,
    SNOWY = 7,
    MUSHROOM_ISLAND = 8,
    BADLANDS = 9,
    COUNT = 10
};

struct BiomeColors {
    float grassR, grassG, grassB;
    float foliageR, foliageG, foliageB;
};

class BiomeSystem {
public:
    // Get biome type from world coordinates using noise
    static BiomeType GetBiomeType(int worldX, int worldZ, int seed);
    
    // Get grass color for a biome (normalized 0-1)
    static void GetGrassColor(BiomeType biome, float& r, float& g, float& b);
    
    // Get foliage/leaf color for a biome (normalized 0-1)
    static void GetFoliageColor(BiomeType biome, float& r, float& g, float& b);
    
    // Convert hex color to normalized RGB (helper function)
    static void HexToRGB(int hexColor, float& r, float& g, float& b);

private:
    // Biome color data based on your specifications
    static const std::array<BiomeColors, static_cast<int>(BiomeType::COUNT)> s_biomeColors;
};