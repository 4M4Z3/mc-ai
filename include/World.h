#pragma once

#include "Chunk.h"
#include "Block.h"
#include "BlockManager.h"
#include <array>
#include <memory>
#include <random>

// World dimensions (6x6 chunks centered around origin)
constexpr int WORLD_SIZE = 6;

class World {
public:
    World();
    explicit World(int seed);
    
    // Block access (world coordinates)
    Block GetBlock(int worldX, int worldY, int worldZ) const;
    void SetBlock(int worldX, int worldY, int worldZ, BlockType type);
    void SetBlock(int worldX, int worldY, int worldZ, const Block& block);
    
    // Efficient block updates for streaming
    void SetBlockWithMeshUpdate(int worldX, int worldY, int worldZ, BlockType type, const BlockManager* blockManager);
    void UpdateNeighboringChunks(int worldX, int worldY, int worldZ, const BlockManager* blockManager);
    void SetBlockBatched(int worldX, int worldY, int worldZ, BlockType type); // Queue block update for batching
    void ProcessAllBatchedUpdates(const BlockManager* blockManager); // Process batched updates across all chunks
    
    // Chunk access
    Chunk* GetChunk(int chunkX, int chunkZ);
    const Chunk* GetChunk(int chunkX, int chunkZ) const;
    
    // World properties
    int GetSeed() const { return m_seed; }
    
    // Generation
    void Generate();
    void GenerateWithBlockManager(const BlockManager* blockManager);
    void RegenerateWithSeed(int newSeed);
    void RegenerateWithSeed(int newSeed, const BlockManager* blockManager);
    
    // Mesh generation
    void GenerateAllMeshes();
    void GenerateAllMeshes(const BlockManager* blockManager);
    void RegenerateMeshes();
    void RegenerateMeshes(const BlockManager* blockManager);
    
    // Utility functions
    bool IsValidWorldPosition(int worldX, int worldY, int worldZ) const;
    void WorldToChunkCoords(int worldX, int worldZ, int& chunkX, int& chunkZ, int& localX, int& localZ) const;
    
    // Find highest non-air block at given world coordinates
    int FindHighestBlock(int worldX, int worldZ) const;

private:
    // 6x6 grid of chunks: [x][z] where x,z are 0 to 5
    // Maps to chunk coordinates: (-3,-3) to (2,2)
    std::array<std::array<std::unique_ptr<Chunk>, WORLD_SIZE>, WORLD_SIZE> m_chunks;
    
    int m_seed;
    std::mt19937 m_randomGenerator;
    
    // Helper functions
    void InitializeChunks();
    bool IsValidChunkIndex(int x, int z) const;
    void ChunkCoordsToArrayIndex(int chunkX, int chunkZ, int& arrayX, int& arrayZ) const;
}; 