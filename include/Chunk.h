#pragma once

#include "Block.h"
#include <array>

// Chunk dimensions
constexpr int CHUNK_WIDTH = 16;
constexpr int CHUNK_HEIGHT = 256;
constexpr int CHUNK_DEPTH = 16;

class Chunk {
public:
    Chunk();
    Chunk(int chunkX, int chunkZ);
    
    // Block access
    Block GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, BlockType type);
    void SetBlock(int x, int y, int z, const Block& block);
    
    // Chunk position
    int GetChunkX() const { return m_chunkX; }
    int GetChunkZ() const { return m_chunkZ; }
    
    // Utility functions
    bool IsValidPosition(int x, int y, int z) const;
    void Fill(BlockType type);
    void Clear();
    
    // Generation
    void Generate(int seed);

private:
    // 3D array of blocks [x][y][z]
    std::array<std::array<std::array<Block, CHUNK_DEPTH>, CHUNK_HEIGHT>, CHUNK_WIDTH> m_blocks;
    
    int m_chunkX;
    int m_chunkZ;
}; 