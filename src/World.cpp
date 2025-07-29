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
    // Initialize 6x6 grid of chunks centered around origin
    // Array index [0][0] = chunk (-3, -3)
    // Array index [3][3] = chunk (0, 0)  
    // Array index [5][5] = chunk (2, 2)
    
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            // Convert array index to chunk coordinates
            // Array indices 0-5 map to chunk coordinates -3 to +2
            int chunkX = x - 3;
            int chunkZ = z - 3;
            
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

void World::SetBlockWithMeshUpdate(int worldX, int worldY, int worldZ, BlockType type) {
    if (!IsValidWorldPosition(worldX, worldY, worldZ)) {
        return;
    }
    
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    Chunk* chunk = GetChunk(chunkX, chunkZ);
    if (chunk) {
        chunk->SetBlock(localX, worldY, localZ, type);
        chunk->UpdateBlockMesh(localX, worldY, localZ, this);
        
        // Update neighboring chunks if block is on chunk boundary
        UpdateNeighboringChunks(worldX, worldY, worldZ);
    }
}

void World::UpdateNeighboringChunks(int worldX, int worldY, int worldZ) {
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    // Check if block is on chunk boundaries
    bool onLeftEdge = (localX == 0);
    bool onRightEdge = (localX == CHUNK_WIDTH - 1);
    bool onBackEdge = (localZ == 0);
    bool onFrontEdge = (localZ == CHUNK_DEPTH - 1);
    
    // Update neighboring chunks that might be affected
    if (onLeftEdge) {
        Chunk* leftChunk = GetChunk(chunkX - 1, chunkZ);
        if (leftChunk) leftChunk->UpdateBlockMesh(CHUNK_WIDTH - 1, worldY, localZ, this);
    }
    
    if (onRightEdge) {
        Chunk* rightChunk = GetChunk(chunkX + 1, chunkZ);
        if (rightChunk) rightChunk->UpdateBlockMesh(0, worldY, localZ, this);
    }
    
    if (onBackEdge) {
        Chunk* backChunk = GetChunk(chunkX, chunkZ - 1);
        if (backChunk) backChunk->UpdateBlockMesh(localX, worldY, CHUNK_DEPTH - 1, this);
    }
    
    if (onFrontEdge) {
        Chunk* frontChunk = GetChunk(chunkX, chunkZ + 1);
        if (frontChunk) frontChunk->UpdateBlockMesh(localX, worldY, 0, this);
    }
}

void World::SetBlockBatched(int worldX, int worldY, int worldZ, BlockType type) {
    if (!IsValidWorldPosition(worldX, worldY, worldZ)) {
        return;
    }
    
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    Chunk* chunk = GetChunk(chunkX, chunkZ);
    if (chunk) {
        BlockType oldType = chunk->GetBlock(localX, worldY, localZ).GetType();
        chunk->BatchBlockUpdate(localX, worldY, localZ, oldType, type);
    }
}

void World::ProcessAllBatchedUpdates() {
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            if (m_chunks[x][z]) {
                m_chunks[x][z]->ProcessBatchedUpdates(this);
            }
        }
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
    
    // Check if position is within our 6x6 chunk world
    int chunkX, chunkZ, localX, localZ;
    WorldToChunkCoords(worldX, worldZ, chunkX, chunkZ, localX, localZ);
    
    // Valid chunk coordinates are -3 to +2 for both X and Z
    return (chunkX >= -3 && chunkX <= 2) && (chunkZ >= -3 && chunkZ <= 2);
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
    // Chunk coordinates -3 to +2 map to array indices 0 to 5
    arrayX = chunkX + 3;
    arrayZ = chunkZ + 3;
} 

int World::FindHighestBlock(int worldX, int worldZ) const {
    // Start from the top and work down to find the highest non-air block
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        Block block = GetBlock(worldX, y, worldZ);
        if (block.GetType() != BlockType::AIR) {
            return y + 1; // Return the Y position above the highest block (where player should spawn)
        }
    }
    
    // If no blocks found, return a default height
    return 64; // Default spawn height if no terrain found
} 