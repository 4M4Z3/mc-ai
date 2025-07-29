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
    // Initialize 2x2 grid of chunks adjacent to each other
    // Array index [0][0] = chunk (0, 0)
    // Array index [0][1] = chunk (0, 1)  
    // Array index [1][0] = chunk (1, 0)
    // Array index [1][1] = chunk (1, 1)
    
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            int chunkX = x;
            int chunkZ = z;
            
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
    
    // Generate meshes after all chunks are generated
    GenerateAllMeshes();
}

void World::RegenerateWithSeed(int newSeed) {
    m_seed = newSeed;
    m_randomGenerator.seed(m_seed);
    
    Generate();
    
    std::cout << "World regenerated with seed: " << m_seed << std::endl;
}

void World::GenerateAllMeshes() {
    // Generate meshes for all chunks
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            if (m_chunks[x][z]) {
                m_chunks[x][z]->GenerateMesh(this);
            }
        }
    }
}

void World::RegenerateMeshes() {
    // Regenerate meshes for all chunks (useful when blocks change)
    GenerateAllMeshes();
}

bool World::IsValidWorldPosition(int worldX, int worldY, int worldZ) const {
    // Check Y bounds
    if (worldY < 0 || worldY >= CHUNK_HEIGHT) {
        return false;
    }
    
    // Check if position is within our 2x2 chunk world
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    // Valid chunk coordinates are 0 and 1 for both X and Z
    return (chunkX >= 0 && chunkX < WORLD_SIZE) && (chunkZ >= 0 && chunkZ < WORLD_SIZE);
}

void World::WorldToChunkCoords(int worldX, int worldZ, int& chunkX, int& chunkZ, int& localX, int& localZ) const {
    // Convert world coordinates to chunk coordinates and local coordinates
    // Each chunk is 16x16, so we divide by chunk size to get chunk coords
    if (worldX >= 0) {
        chunkX = worldX / CHUNK_WIDTH;
        localX = worldX % CHUNK_WIDTH;
    } else {
        // Handle negative coordinates
        chunkX = (worldX - CHUNK_WIDTH + 1) / CHUNK_WIDTH;
        localX = worldX - (chunkX * CHUNK_WIDTH);
    }
    
    if (worldZ >= 0) {
        chunkZ = worldZ / CHUNK_DEPTH;
        localZ = worldZ % CHUNK_DEPTH;
    } else {
        // Handle negative coordinates
        chunkZ = (worldZ - CHUNK_DEPTH + 1) / CHUNK_DEPTH;
        localZ = worldZ - (chunkZ * CHUNK_DEPTH);
    }
}

bool World::IsValidChunkIndex(int x, int z) const {
    return x >= 0 && x < WORLD_SIZE && z >= 0 && z < WORLD_SIZE;
}

void World::ChunkCoordsToArrayIndex(int chunkX, int chunkZ, int& arrayX, int& arrayZ) const {
    // Convert chunk coordinates to array indices
    // Now chunk coordinates directly map to array indices
    arrayX = chunkX;
    arrayZ = chunkZ;
} 