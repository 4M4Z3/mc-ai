#include "Chunk.h"

Chunk::Chunk() : m_chunkX(0), m_chunkZ(0) {
    Clear();
}

Chunk::Chunk(int chunkX, int chunkZ) : m_chunkX(chunkX), m_chunkZ(chunkZ) {
    Clear();
}

Block Chunk::GetBlock(int x, int y, int z) const {
    if (!IsValidPosition(x, y, z)) {
        return Block(BlockType::AIR);
    }
    return m_blocks[x][y][z];
}

void Chunk::SetBlock(int x, int y, int z, BlockType type) {
    if (!IsValidPosition(x, y, z)) {
        return;
    }
    m_blocks[x][y][z].SetType(type);
}

void Chunk::SetBlock(int x, int y, int z, const Block& block) {
    if (!IsValidPosition(x, y, z)) {
        return;
    }
    m_blocks[x][y][z] = block;
}

bool Chunk::IsValidPosition(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_WIDTH &&
           y >= 0 && y < CHUNK_HEIGHT &&
           z >= 0 && z < CHUNK_DEPTH;
}

void Chunk::Fill(BlockType type) {
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                m_blocks[x][y][z].SetType(type);
            }
        }
    }
}

void Chunk::Clear() {
    Fill(BlockType::AIR);
}

void Chunk::Generate(int seed) {
    // Fill all blocks with air first
    Clear();
    
    // Only place a single block at local position 0,0,0 in the chunk that contains world coordinate 0,0,0
    // World coordinate (0,0,0) is in chunk (1,1) at local position (0,0,0)
    if (m_chunkX == 1 && m_chunkZ == 1) {
        m_blocks[0][0][0].SetType(BlockType::BLOCK);
    }
} 