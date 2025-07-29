#include "World.h"
#include <chrono>
#include <iostream>

World::World() {
    // Generate random seed
    m_seed = static_cast<int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    m_randomGenerator.seed(m_seed);
    
    InitializeChunks();
    Generate();
    
    std::cout << "World created with seed: " << m_seed << std::endl;
}

World::World(int seed) : m_seed(seed) {
    m_randomGenerator.seed(m_seed);
    
    InitializeChunks();
    Generate();
    
    std::cout << "World created with seed: " << m_seed << std::endl;
}

void World::InitializeChunks() {
    // Initialize 2x2 grid of chunks
    // Array index [0][0] = chunk (-1, -1)
    // Array index [0][1] = chunk (-1,  1)  
    // Array index [1][0] = chunk ( 1, -1)
    // Array index [1][1] = chunk ( 1,  1)
    
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            int chunkX = (x == 0) ? -1 : 1;
            int chunkZ = (z == 0) ? -1 : 1;
            
            m_chunks[x][z] = std::make_unique<Chunk>(chunkX, chunkZ);
        }
    }
}

Block World::GetBlock(int worldX, int worldY, int worldZ) const {
    if (!IsValidWorldPosition(worldX, worldY, worldZ)) {
        return Block(BlockType::AIR);
    }
    
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    const Chunk* chunk = GetChunk(chunkX, chunkZ);
    if (!chunk) {
        return Block(BlockType::AIR);
    }
    
    return chunk->GetBlock(localX, worldY, localZ);
}

void World::SetBlock(int worldX, int worldY, int worldZ, BlockType type) {
    if (!IsValidWorldPosition(worldX, worldY, worldZ)) {
        return;
    }
    
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    Chunk* chunk = GetChunk(chunkX, chunkZ);
    if (chunk) {
        chunk->SetBlock(localX, worldY, localZ, type);
    }
}

void World::SetBlock(int worldX, int worldY, int worldZ, const Block& block) {
    if (!IsValidWorldPosition(worldX, worldY, worldZ)) {
        return;
    }
    
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    Chunk* chunk = GetChunk(chunkX, chunkZ);
    if (chunk) {
        chunk->SetBlock(localX, worldY, localZ, block);
    }
}

Chunk* World::GetChunk(int chunkX, int chunkZ) {
    int arrayX, arrayZ;
    ChunkCoordsToArrayIndex(chunkX, chunkZ, arrayX, arrayZ);
    
    if (!IsValidChunkIndex(arrayX, arrayZ)) {
        return nullptr;
    }
    
    return m_chunks[arrayX][arrayZ].get();
}

const Chunk* World::GetChunk(int chunkX, int chunkZ) const {
    int arrayX, arrayZ;
    ChunkCoordsToArrayIndex(chunkX, chunkZ, arrayX, arrayZ);
    
    if (!IsValidChunkIndex(arrayX, arrayZ)) {
        return nullptr;
    }
    
    return m_chunks[arrayX][arrayZ].get();
}

void World::Generate() {
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            if (m_chunks[x][z]) {
                m_chunks[x][z]->Generate(m_seed);
            }
        }
    }
}

void World::RegenerateWithSeed(int newSeed) {
    m_seed = newSeed;
    m_randomGenerator.seed(m_seed);
    
    Generate();
    
    std::cout << "World regenerated with seed: " << m_seed << std::endl;
}

bool World::IsValidWorldPosition(int worldX, int worldY, int worldZ) const {
    // Check Y bounds
    if (worldY < 0 || worldY >= CHUNK_HEIGHT) {
        return false;
    }
    
    // Check if position is within our 2x2 chunk world
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    // Valid chunk coordinates are -1 and 1 for both X and Z
    return (chunkX == -1 || chunkX == 1) && (chunkZ == -1 || chunkZ == 1);
}

void World::WorldToChunkCoords(int worldX, int worldZ, int& chunkX, int& chunkZ, int& localX, int& localZ) const {
    // Convert world coordinates to chunk coordinates and local coordinates
    if (worldX >= 0) {
        chunkX = 1;
        localX = worldX % CHUNK_WIDTH;
    } else {
        chunkX = -1;
        localX = (CHUNK_WIDTH + (worldX % CHUNK_WIDTH)) % CHUNK_WIDTH;
    }
    
    if (worldZ >= 0) {
        chunkZ = 1;
        localZ = worldZ % CHUNK_DEPTH;
    } else {
        chunkZ = -1;
        localZ = (CHUNK_DEPTH + (worldZ % CHUNK_DEPTH)) % CHUNK_DEPTH;
    }
}

bool World::IsValidChunkIndex(int x, int z) const {
    return x >= 0 && x < WORLD_SIZE && z >= 0 && z < WORLD_SIZE;
}

void World::ChunkCoordsToArrayIndex(int chunkX, int chunkZ, int& arrayX, int& arrayZ) const {
    // Convert chunk coordinates to array indices
    // chunkX: -1 -> 0, 1 -> 1
    // chunkZ: -1 -> 0, 1 -> 1
    arrayX = (chunkX == -1) ? 0 : 1;
    arrayZ = (chunkZ == -1) ? 0 : 1;
} 