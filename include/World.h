#pragma once

#include "Chunk.h"
#include "Block.h"
#include <array>
#include <memory>
#include <random>

// World dimensions (2x2 chunks around origin)
constexpr int WORLD_SIZE = 2;

class World {
public:
    World();
    explicit World(int seed);
    
    // Block access (world coordinates)
    Block GetBlock(int worldX, int worldY, int worldZ) const;
    void SetBlock(int worldX, int worldY, int worldZ, BlockType type);
    void SetBlock(int worldX, int worldY, int worldZ, const Block& block);
    
    // Chunk access
    Chunk* GetChunk(int chunkX, int chunkZ);
    const Chunk* GetChunk(int chunkX, int chunkZ) const;
    
    // World properties
    int GetSeed() const { return m_seed; }
    
    // Generation
    void Generate();
    void RegenerateWithSeed(int newSeed);
    
    // Utility functions
    bool IsValidWorldPosition(int worldX, int worldY, int worldZ) const;
    void WorldToChunkCoords(int worldX, int worldZ, int& chunkX, int& chunkZ, int& localX, int& localZ) const;
    
private:
    // 2x2 grid of chunks: [x][z] where x,z are 0 or 1
    // Maps to chunk coordinates: (-1,-1), (-1,1), (1,-1), (1,1)
    std::array<std::array<std::unique_ptr<Chunk>, WORLD_SIZE>, WORLD_SIZE> m_chunks;
    
    int m_seed;
    std::mt19937 m_randomGenerator;
    
    // Helper functions
    void InitializeChunks();
    bool IsValidChunkIndex(int x, int z) const;
    void ChunkCoordsToArrayIndex(int chunkX, int chunkZ, int& arrayX, int& arrayZ) const;
}; 