#pragma once

#include <array>

enum class BiomeType : int {
    // Cold biomes
    SNOWY_TUNDRA = 0,
    TAIGA = 1,
    SNOWY_TAIGA = 2,
    
    // Temperate biomes
    FOREST = 3,
    PLAINS = 4,
    SWAMP = 5,
    
    // Warm biomes
    JUNGLE = 6,
    SAVANNA = 7,
    DESERT = 8,
    
    // Special biomes
    MUSHROOM_ISLAND = 9,
    BADLANDS = 10,
    RIVER = 11,
    
    COUNT = 12
};

struct BiomeColors {
    float grassR, grassG, grassB;
    float foliageR, foliageG, foliageB;
};

struct TemperatureHumidity {
    float temperature; // 0.0 = cold, 1.0 = hot
    float humidity;    // 0.0 = dry, 1.0 = wet
};

class BiomeSystem {
public:
    // Get biome type from world coordinates using temperature and humidity
    static BiomeType GetBiomeType(int worldX, int worldZ, int seed);
    
    // Get temperature and humidity at world coordinates
    static TemperatureHumidity GetTemperatureHumidity(int worldX, int worldZ, int seed);
    
    // Determine biome from temperature and humidity values
    static BiomeType GetBiomeFromClimate(float temperature, float humidity);
    
    // Check if location should be a river
    static bool IsRiver(int worldX, int worldZ, int seed);
    
    // Get grass color for a biome (normalized 0-1)
    static void GetGrassColor(BiomeType biome, float& r, float& g, float& b);
    
    // Get foliage/leaf color for a biome (normalized 0-1)
    static void GetFoliageColor(BiomeType biome, float& r, float& g, float& b);
    
    // Convert hex color to normalized RGB (helper function)
    static void HexToRGB(int hexColor, float& r, float& g, float& b);

private:
    // Biome color data based on your specifications
    static const std::array<BiomeColors, static_cast<int>(BiomeType::COUNT)> s_biomeColors;
    
    // Noise functions for climate generation
    static double PerlinNoise(double x, double z, int seed, double scale);
    static double SimplexNoise(double x, double z, int seed, double scale);
};