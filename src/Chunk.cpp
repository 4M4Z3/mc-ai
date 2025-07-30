#include "Chunk.h"
#include "World.h"
#include "BiomeSystem.h"
#include <cmath>
#include <iostream>
#include <unordered_map>

Chunk::Chunk() : m_chunkX(0), m_chunkZ(0), m_meshGenerated(false) {
    Clear();
}

Chunk::Chunk(int chunkX, int chunkZ) : m_chunkX(chunkX), m_chunkZ(chunkZ), m_meshGenerated(false) {
    Clear();
}

Chunk::~Chunk() {
    ClearMesh();
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
    // Mark mesh as dirty when blocks change
    m_meshGenerated = false;
}

void Chunk::SetBlock(int x, int y, int z, const Block& block) {
    if (!IsValidPosition(x, y, z)) {
        return;
    }
    m_blocks[x][y][z] = block;
    // Mark mesh as dirty when blocks change
    m_meshGenerated = false;
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
    m_meshGenerated = false;
}

void Chunk::Clear() {
    Fill(BlockType::AIR);
}

void Chunk::ClearMesh() {
    for (auto& pair : m_blockMeshes) {
        BlockMesh& mesh = pair.second;
        if (mesh.VAO) {
            glDeleteVertexArrays(1, &mesh.VAO);
            mesh.VAO = 0;
        }
        if (mesh.VBO) {
            glDeleteBuffers(1, &mesh.VBO);
            mesh.VBO = 0;
        }
        mesh.vertexCount = 0;
    }
    m_blockMeshes.clear();
    
    // Clear grass face meshes too
    for (auto& pair : m_grassFaceMeshes) {
        BlockMesh& mesh = pair.second;
        if (mesh.VAO) {
            glDeleteVertexArrays(1, &mesh.VAO);
            mesh.VAO = 0;
        }
        if (mesh.VBO) {
            glDeleteBuffers(1, &mesh.VBO);
            mesh.VBO = 0;
        }
        mesh.vertexCount = 0;
    }
    m_grassFaceMeshes.clear();
    
    // Clear log face meshes too
    for (auto& pair : m_logFaceMeshes) {
        BlockMesh& mesh = pair.second;
        if (mesh.VAO) {
            glDeleteVertexArrays(1, &mesh.VAO);
            mesh.VAO = 0;
        }
        if (mesh.VBO) {
            glDeleteBuffers(1, &mesh.VBO);
            mesh.VBO = 0;
        }
        mesh.vertexCount = 0;
    }
    m_logFaceMeshes.clear();
    
    m_meshGenerated = false;
}

void Chunk::GenerateMesh(const World* world, const BlockManager* blockManager) {
    // Clear existing meshes
    ClearMesh();
    
    // Group vertices by block type
    std::unordered_map<BlockType, std::vector<float>> blockVertices;
    
    // Separate vertex groups for grass faces
    std::unordered_map<GrassFaceType, std::vector<float>> grassFaceVertices;
    
    // Separate vertex groups for log faces (similar to grass)
    std::unordered_map<GrassFaceType, std::vector<float>> logFaceVertices;
    
    // Generate mesh data for all non-air blocks, grouped by type
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                BlockType blockType = m_blocks[x][y][z].GetType();
                if (blockType != BlockType::AIR) {
                    // Check if this is a ground block (should render as cross)
                    if (blockManager && blockManager->IsGround(blockType)) {
                        // Render ground blocks as diagonal cross sprites
                        AddCrossToMesh(blockVertices[blockType], x, y, z, world);
                    } else {
                        // Check each face for visibility (standard cube rendering)
                        for (int face = 0; face < 6; ++face) {
                            if (ShouldRenderFace(x, y, z, face, world, blockManager)) {
                            // Handle grass blocks specially
                            if (blockType == BlockType::GRASS) {
                                // Group grass faces by face type for different textures
                                if (face == FACE_TOP) {
                                    AddFaceToMesh(grassFaceVertices[GRASS_TOP], x, y, z, face, world);
                                } else if (face == FACE_BOTTOM) {
                                    AddFaceToMesh(grassFaceVertices[GRASS_BOTTOM], x, y, z, face, world);
                                } else {
                                    // Side faces (FRONT, BACK, LEFT, RIGHT) - flip texture vertically  
                                    AddFaceToMesh(grassFaceVertices[GRASS_SIDE], x, y, z, face, world, true);
                                }
                            } else if (blockType == BlockType::OAK_LOG || blockType == BlockType::BIRCH_LOG || blockType == BlockType::DARK_OAK_LOG) {
                                // Handle log blocks - top/bottom use different texture than sides
                                if (face == FACE_TOP || face == FACE_BOTTOM) {
                                    // Top and bottom faces use log_top texture
                                    AddFaceToMesh(logFaceVertices[GRASS_TOP], x, y, z, face, world);
                                } else {
                                    // Side faces (FRONT, BACK, LEFT, RIGHT) use log side texture
                                    AddFaceToMesh(logFaceVertices[GRASS_SIDE], x, y, z, face, world);
                                }
                            } else {
                                // Add face vertices to the appropriate block type group
                                AddFaceToMesh(blockVertices[blockType], x, y, z, face, world);
                            }
                        }
                    }
                    } // end else (standard cube rendering)
                }
            }
        }
    }
    
    // Create OpenGL meshes for each block type that has vertices
    for (auto& pair : blockVertices) {
        BlockType blockType = pair.first;
        std::vector<float>& vertices = pair.second;
        
        if (vertices.empty()) {
            continue;
        }
        
        BlockMesh& mesh = m_blockMeshes[blockType];
        
        // Create OpenGL mesh
        glGenVertexArrays(1, &mesh.VAO);
        glBindVertexArray(mesh.VAO);
        
        glGenBuffers(1, &mesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        // Position attribute (x, y, z) - location 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Ambient occlusion attribute (ao) - location 1
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Texture coordinate attribute (u, v) - location 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        mesh.vertexCount = vertices.size() / 6; // 6 floats per vertex
    }
    
    // Create OpenGL meshes for grass faces that have vertices
    for (auto& pair : grassFaceVertices) {
        GrassFaceType faceType = pair.first;
        std::vector<float>& vertices = pair.second;
        
        if (vertices.empty()) {
            continue;
        }
        
        BlockMesh& mesh = m_grassFaceMeshes[faceType];
        
        // Create OpenGL mesh
        glGenVertexArrays(1, &mesh.VAO);
        glBindVertexArray(mesh.VAO);
        
        glGenBuffers(1, &mesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        // Position attribute (x, y, z) - location 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Ambient occlusion attribute (ao) - location 1
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Texture coordinate attribute (u, v) - location 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        mesh.vertexCount = vertices.size() / 6; // 6 floats per vertex
    }
    
    // Create OpenGL meshes for log faces that have vertices
    for (auto& pair : logFaceVertices) {
        GrassFaceType faceType = pair.first;
        std::vector<float>& vertices = pair.second;
        
        if (vertices.empty()) {
            continue;
        }
        
        BlockMesh& mesh = m_logFaceMeshes[faceType];
        
        // Create OpenGL mesh
        glGenVertexArrays(1, &mesh.VAO);
        glBindVertexArray(mesh.VAO);
        
        glGenBuffers(1, &mesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        // Position attribute (x, y, z) - location 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Ambient occlusion attribute (ao) - location 1
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Texture coordinate attribute (u, v) - location 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        mesh.vertexCount = vertices.size() / 6; // 6 floats per vertex
    }
    
    m_meshGenerated = true;
}

void Chunk::UpdateBlockMesh(int x, int y, int z, const World* world, const BlockManager* blockManager) {
    // For now, fall back to full mesh regeneration
    // TODO: Implement truly incremental mesh updates
    GenerateMesh(world, blockManager);
}

void Chunk::BatchBlockUpdate(int x, int y, int z, BlockType oldType, BlockType newType) {
    PendingBlockUpdate update;
    update.x = x;
    update.y = y;
    update.z = z;
    update.oldType = oldType;
    update.newType = newType;
    
    m_pendingUpdates.push_back(update);
    m_hasPendingUpdates = true;
}

void Chunk::ProcessBatchedUpdates(const World* world, const BlockManager* blockManager) {
    if (!m_hasPendingUpdates || m_pendingUpdates.empty()) {
        return;
    }
    
    std::cout << "[CHUNK] Processing " << m_pendingUpdates.size() << " batched block updates for chunk (" 
              << m_chunkX << ", " << m_chunkZ << ")" << std::endl;
    
    // Apply all pending updates to the chunk
    for (const auto& update : m_pendingUpdates) {
        if (IsValidPosition(update.x, update.y, update.z)) {
            m_blocks[update.x][update.y][update.z].SetType(update.newType);
        }
    }
    
    // Clear pending updates
    m_pendingUpdates.clear();
    m_hasPendingUpdates = false;
    
    // Regenerate mesh once for all updates with proper BlockManager
    GenerateMesh(world, blockManager);
    
    std::cout << "[CHUNK] Completed batched mesh update for chunk (" << m_chunkX << ", " << m_chunkZ << ")" << std::endl;
}

void Chunk::RenderMesh() const {
    // This method now renders all block types - but we'll change this approach
    for (const auto& pair : m_blockMeshes) {
        const BlockMesh& mesh = pair.second;
        if (mesh.VAO != 0 && mesh.vertexCount > 0) {
            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            glBindVertexArray(0);
        }
    }
}

void Chunk::RenderMeshForBlockType(BlockType blockType) const {
    auto it = m_blockMeshes.find(blockType);
    if (it != m_blockMeshes.end()) {
        const BlockMesh& mesh = it->second;
        if (mesh.VAO != 0 && mesh.vertexCount > 0) {
            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            glBindVertexArray(0);
        }
    }
}

void Chunk::RenderGrassMesh(GrassFaceType faceType) const {
    auto it = m_grassFaceMeshes.find(faceType);
    if (it != m_grassFaceMeshes.end()) {
        const BlockMesh& mesh = it->second;
        if (mesh.VAO != 0 && mesh.vertexCount > 0) {
            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            glBindVertexArray(0);
        }
    }
}

void Chunk::RenderLogMesh(GrassFaceType faceType) const {
    auto it = m_logFaceMeshes.find(faceType);
    if (it != m_logFaceMeshes.end()) {
        const BlockMesh& mesh = it->second;
        if (mesh.VAO != 0 && mesh.vertexCount > 0) {
            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
            glBindVertexArray(0);
        }
    }
}

std::vector<BlockType> Chunk::GetBlockTypesInChunk() const {
    std::vector<BlockType> blockTypes;
    for (const auto& pair : m_blockMeshes) {
        if (pair.second.vertexCount > 0) {
            blockTypes.push_back(pair.first);
        }
    }
    return blockTypes;
}

bool Chunk::ShouldRenderFace(int x, int y, int z, int faceDirection, const World* world, const BlockManager* blockManager) const {
    Block neighbor = GetNeighborBlock(x, y, z, faceDirection, world);
    BlockType currentBlockType = m_blocks[x][y][z].GetType();
    
    // Always show faces adjacent to air
    if (neighbor.IsAir()) {
        return true;
    }
    
    // Special handling for water blocks
    if (currentBlockType == BlockType::WATER_STILL || currentBlockType == BlockType::WATER_FLOW) {
        BlockType neighborType = neighbor.GetType();
        // For water: only render faces that are NOT adjacent to other water blocks
        return neighborType != BlockType::WATER_STILL && neighborType != BlockType::WATER_FLOW;
    }
    
    // If we have a BlockManager, also show faces adjacent to transparent or ground blocks
    if (blockManager) {
        BlockType neighborType = neighbor.GetType();
        
        // Special case: always render faces adjacent to water blocks (regardless of transparency detection)
        if (neighborType == BlockType::WATER_STILL || neighborType == BlockType::WATER_FLOW) {
            return true;
        }
        
        return blockManager->IsTransparent(neighborType) || blockManager->IsGround(neighborType);
    }
    
    // Fallback to original behavior if no BlockManager
    return false;
}

Block Chunk::GetNeighborBlock(int x, int y, int z, int faceDirection, const World* world) const {
    int neighborX = x;
    int neighborY = y;
    int neighborZ = z;
    
    // Calculate neighbor position based on face direction
    switch (faceDirection) {
        case FACE_FRONT:  neighborZ++; break;  // +Z
        case FACE_BACK:   neighborZ--; break;  // -Z
        case FACE_LEFT:   neighborX--; break;  // -X
        case FACE_RIGHT:  neighborX++; break;  // +X
        case FACE_BOTTOM: neighborY--; break;  // -Y
        case FACE_TOP:    neighborY++; break;  // +Y
    }
    
    // If neighbor is within this chunk, get it directly
    if (IsValidPosition(neighborX, neighborY, neighborZ)) {
        return m_blocks[neighborX][neighborY][neighborZ];
    }
    
    // Neighbor is outside this chunk - convert to world coordinates and query world
    if (world) {
        int worldX = m_chunkX * CHUNK_WIDTH + neighborX;
        int worldY = neighborY;
        int worldZ = m_chunkZ * CHUNK_DEPTH + neighborZ;
        
        return world->GetBlock(worldX, worldY, worldZ);
    }
    
    // No world access - assume air (this shouldn't happen in normal usage)
    return Block(BlockType::AIR);
}

void Chunk::AddFaceToMesh(std::vector<float>& vertices, int x, int y, int z, int faceDirection, const World* world, bool flipTextureV) const {
    BlockType currentBlockType = m_blocks[x][y][z].GetType();
    // Convert local chunk coordinates to world position for rendering
    float worldX = static_cast<float>(m_chunkX * CHUNK_WIDTH + x);
    float worldY = static_cast<float>(y);
    float worldZ = static_cast<float>(m_chunkZ * CHUNK_DEPTH + z);
    
    // Calculate texture coordinates for top and bottom based on flip setting
    float vBottom = flipTextureV ? 1.0f : 0.0f;
    float vTop = flipTextureV ? 0.0f : 1.0f;
    
    // Face vertices with position (3), AO (1), and texture coordinates (2) 
    // Each vertex: x, y, z, ao_value, u, v (6 floats per vertex)
    
    switch (faceDirection) {
        case FACE_FRONT: { // +Z face
            // Calculate AO for each vertex of the front face
            float ao0 = CalculateVertexAO(x, y, z, FACE_FRONT, 0, world); // Bottom-left
            float ao1 = CalculateVertexAO(x, y, z, FACE_FRONT, 1, world); // Bottom-right  
            float ao2 = CalculateVertexAO(x, y, z, FACE_FRONT, 2, world); // Top-right
            float ao3 = CalculateVertexAO(x, y, z, FACE_FRONT, 3, world); // Top-left
            
            float frontVertices[] = {
                // Triangle 1: x, y, z, ao, u, v
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao0, 0.0f, vBottom, // Bottom-left
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1, 1.0f, vBottom, // Bottom-right
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 1.0f, vTop, // Top-right
                // Triangle 2  
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 1.0f, vTop, // Top-right
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao3, 0.0f, vTop, // Top-left
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao0, 0.0f, vBottom  // Bottom-left
            };
            vertices.insert(vertices.end(), frontVertices, frontVertices + 36);
            break;
        }
        case FACE_BACK: { // -Z face
            float ao0 = CalculateVertexAO(x, y, z, FACE_BACK, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_BACK, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_BACK, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_BACK, 3, world);
            
            float backVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 1.0f, vBottom,
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3, 1.0f, vTop,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao2, 0.0f, vTop,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao2, 0.0f, vTop,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao1, 0.0f, vBottom,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 1.0f, vBottom
            };
            vertices.insert(vertices.end(), backVertices, backVertices + 36);
            break;
        }
        case FACE_LEFT: { // -X face
            float ao0 = CalculateVertexAO(x, y, z, FACE_LEFT, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_LEFT, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_LEFT, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_LEFT, 3, world);
            
            float leftVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 0.0f, vBottom,
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1, 1.0f, vBottom,
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 1.0f, vTop,
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 1.0f, vTop,
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3, 0.0f, vTop,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 0.0f, vBottom
            };
            vertices.insert(vertices.end(), leftVertices, leftVertices + 36);
            break;
        }
        case FACE_RIGHT: { // +X face
            float ao0 = CalculateVertexAO(x, y, z, FACE_RIGHT, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_RIGHT, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_RIGHT, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_RIGHT, 3, world);
            
            float rightVertices[] = {
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 1.0f, vBottom,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3, 1.0f, vTop,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 0.0f, vTop,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, 0.0f, vTop,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1, 0.0f, vBottom,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 1.0f, vBottom
            };
            vertices.insert(vertices.end(), rightVertices, rightVertices + 36);
            break;
        }
        case FACE_BOTTOM: { // -Y face
            float ao0 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 3, world);
            
            float bottomVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 0.0f, 0.0f,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao1, 1.0f, 0.0f,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao2, 1.0f, 1.0f,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao2, 1.0f, 1.0f,
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao3, 0.0f, 1.0f,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0, 0.0f, 0.0f
            };
            vertices.insert(vertices.end(), bottomVertices, bottomVertices + 36);
            break;
        }
        case FACE_TOP: { // +Y face
            float ao0 = CalculateVertexAO(x, y, z, FACE_TOP, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_TOP, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_TOP, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_TOP, 3, world);
            
            // Water blocks have lowered surface at 15/16 height (0.9375)
            float topY = worldY + 0.5f;
            if (currentBlockType == BlockType::WATER_STILL || currentBlockType == BlockType::WATER_FLOW) {
                topY = worldY - 0.5f + 0.9375f; // 15/16 height from block bottom
            }
            
            float topVertices[] = {
                worldX - 0.5f, topY, worldZ - 0.5f, ao0, 0.0f, 0.0f,
                worldX - 0.5f, topY, worldZ + 0.5f, ao3, 1.0f, 0.0f,
                worldX + 0.5f, topY, worldZ + 0.5f, ao2, 1.0f, 1.0f,
                worldX + 0.5f, topY, worldZ + 0.5f, ao2, 1.0f, 1.0f,
                worldX + 0.5f, topY, worldZ - 0.5f, ao1, 0.0f, 1.0f,
                worldX - 0.5f, topY, worldZ - 0.5f, ao0, 0.0f, 0.0f
            };
            vertices.insert(vertices.end(), topVertices, topVertices + 36);
            break;
        }
    }
}

void Chunk::AddCrossToMesh(std::vector<float>& vertices, int x, int y, int z, const World* world) const {
    // Convert local chunk coordinates to world position for rendering
    float worldX = static_cast<float>(m_chunkX * CHUNK_WIDTH + x);
    float worldY = static_cast<float>(y);
    float worldZ = static_cast<float>(m_chunkZ * CHUNK_DEPTH + z);
    
    // For cross sprites, we don't need complex AO calculation, use a simple value
    float ao = 1.0f; // Full brightness for plants
    
    // Create four quads (two diagonal planes, each with front and back faces) to form a cross
    // Each plane shows the full texture (0,0) to (1,1)
    
    // First diagonal plane FRONT FACE: from bottom-left-back to top-right-front
    float plane1FrontVertices[] = {
        // Triangle 1: x, y, z, ao, u, v
        worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 0.0f, 1.0f, // Bottom-left-back
        worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 1.0f, 1.0f, // Bottom-right-front
        worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 1.0f, 0.0f, // Top-right-front
        // Triangle 2
        worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 1.0f, 0.0f, // Top-right-front
        worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 0.0f, 0.0f, // Top-left-back
        worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 0.0f, 1.0f  // Bottom-left-back
    };
    
    // First diagonal plane BACK FACE: reverse winding order for backface culling
    float plane1BackVertices[] = {
        // Triangle 1: x, y, z, ao, u, v (reversed winding)
        worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 0.0f, 1.0f, // Bottom-left-back
        worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 0.0f, 0.0f, // Top-left-back
        worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 1.0f, 0.0f, // Top-right-front
        // Triangle 2
        worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 1.0f, 0.0f, // Top-right-front
        worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 1.0f, 1.0f, // Bottom-right-front
        worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 0.0f, 1.0f  // Bottom-left-back
    };
    
    // Second diagonal plane FRONT FACE: from bottom-left-front to top-right-back
    float plane2FrontVertices[] = {
        // Triangle 1: x, y, z, ao, u, v
        worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 0.0f, 1.0f, // Bottom-left-front
        worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 0.0f, 0.0f, // Top-left-front
        worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 1.0f, 0.0f, // Top-right-back
        // Triangle 2
        worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 1.0f, 0.0f, // Top-right-back
        worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 1.0f, 1.0f, // Bottom-right-back
        worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 0.0f, 1.0f  // Bottom-left-front
    };
    
    // Second diagonal plane BACK FACE: reverse winding order for backface culling
    float plane2BackVertices[] = {
        // Triangle 1: x, y, z, ao, u, v (reversed winding)
        worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 0.0f, 1.0f, // Bottom-left-front
        worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 1.0f, 0.0f, // Top-right-back
        worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao, 0.0f, 0.0f, // Top-left-front
        // Triangle 2
        worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao, 1.0f, 0.0f, // Top-right-back
        worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao, 0.0f, 1.0f, // Bottom-left-front
        worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao, 1.0f, 1.0f  // Bottom-right-back
    };
    
    // Add all four quads to the vertex buffer
    vertices.insert(vertices.end(), plane1FrontVertices, plane1FrontVertices + 36); // 6 vertices * 6 floats each
    vertices.insert(vertices.end(), plane1BackVertices, plane1BackVertices + 36);   // 6 vertices * 6 floats each
    vertices.insert(vertices.end(), plane2FrontVertices, plane2FrontVertices + 36); // 6 vertices * 6 floats each
    vertices.insert(vertices.end(), plane2BackVertices, plane2BackVertices + 36);   // 6 vertices * 6 floats each
}

void Chunk::Generate(int seed, const BlockManager* blockManager) {
    // Fill all blocks with air first
    Clear();
    
    // Set up random number generator for block selection
    std::mt19937 rng(seed + m_chunkX * 1000 + m_chunkZ);
    
    // Get available surface blocks (only blocks that have textures)
    std::vector<BlockType> surfaceBlocks;
    
    if (blockManager) {
        // Get all available block types
        std::vector<BlockType> allBlocks = blockManager->GetAllBlockTypes();
        
        // Add specific blocks that we know have good textures
        std::vector<BlockType> knownGoodBlocks = {
            BlockType::STONE, BlockType::DIRT, BlockType::GRASS,
            static_cast<BlockType>(4),  // ACACIA_LEAVES  
            static_cast<BlockType>(5),  // ACACIA_PLANKS
            static_cast<BlockType>(6),  // ACACIA_SAPLING
            static_cast<BlockType>(7),  // ALLIUM
            static_cast<BlockType>(8),  // AMETHYST_BLOCK
            static_cast<BlockType>(9),  // AMETHYST_CLUSTER
            static_cast<BlockType>(10), // ANDESITE
            static_cast<BlockType>(11), // AZALEA_LEAVES
            static_cast<BlockType>(12), // AZALEA_PLANT
            static_cast<BlockType>(13), // AZURE_BLUET
            static_cast<BlockType>(14), // BEACON
            static_cast<BlockType>(15), // BEDROCK
            static_cast<BlockType>(16), // BIRCH_LEAVES
            static_cast<BlockType>(17), // BIRCH_PLANKS
            static_cast<BlockType>(18), // BIRCH_SAPLING
            static_cast<BlockType>(26), // BRICKS  
            static_cast<BlockType>(44), // COPPER_BLOCK
            static_cast<BlockType>(68), // EMERALD_BLOCK
            static_cast<BlockType>(73), // GOLD_BLOCK
            static_cast<BlockType>(96), // IRON_BLOCK
        };
        
        // Add the known good blocks
        for (BlockType blockType : knownGoodBlocks) {
            surfaceBlocks.push_back(blockType);
        }
        
        // Debug output to see what we found
        // std::cout << "[CHUNK DEBUG] Found " << surfaceBlocks.size() << " surface blocks for chunk (" 
        //           << m_chunkX << "," << m_chunkZ << ")" << std::endl;
    }
    
    // Fallback to classic blocks if BlockManager is not available or no suitable blocks found
    if (surfaceBlocks.empty()) {
        surfaceBlocks = {BlockType::GRASS, BlockType::STONE, BlockType::DIRT};
    }
    
    // Generate terrain using multiple octaves of noise for varied geography
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            // Convert local chunk coordinates to world coordinates
            int worldX = m_chunkX * CHUNK_WIDTH + x;
            int worldZ = m_chunkZ * CHUNK_DEPTH + z;
            
            // Get biome for this position using the new BiomeSystem
            BiomeType biomeType = BiomeSystem::GetBiomeType(worldX, worldZ, seed);
            
            // Generate height using multiple noise octaves for varied terrain
            double coarseNoise = Perlin(worldX * NOISE_SCALE_COARSE, worldZ * NOISE_SCALE_COARSE, seed);
            double mediumNoise = Perlin(worldX * NOISE_SCALE, worldZ * NOISE_SCALE, seed + 1000);
            double fineNoise = Perlin(worldX * NOISE_SCALE_FINE, worldZ * NOISE_SCALE_FINE, seed + 2000);
            
            // Combine noise octaves with different weights for varied terrain
            double combinedNoise = coarseNoise * 0.6 + mediumNoise * 0.3 + fineNoise * 0.1;
            
            // Map noise from [-1, 1] to [0, 1] and scale to height variation
            double normalizedNoise = (combinedNoise + 1.0) * 0.5;
            int terrainHeight = BASE_HEIGHT + static_cast<int>(normalizedNoise * MAX_HEIGHT_VARIATION);
            
            // Ensure terrain doesn't generate underwater - enforce minimum height above sea level
            terrainHeight = std::max(SEA_LEVEL + 1, terrainHeight);
            
            // Clamp height to valid range
            terrainHeight = std::max(0, std::min(terrainHeight, CHUNK_HEIGHT - 1));
            
            // Handle rivers - lower the terrain and add water
            if (biomeType == BiomeType::RIVER) {
                // Rivers are carved below sea level
                terrainHeight = std::max(SEA_LEVEL - 3, std::min(terrainHeight - 8, SEA_LEVEL - 1));
            }
            
            // Fill blocks from bottom up to terrain height
            for (int y = 0; y <= terrainHeight; ++y) {
                if (y == terrainHeight) {
                    // Top layer: biome-specific surface blocks
                    BlockType surfaceBlock;
                    
                    switch (biomeType) {
                        case BiomeType::DESERT:
                        case BiomeType::SAVANNA:
                            surfaceBlock = BlockType::SAND;
                            break;
                        case BiomeType::SNOWY_TUNDRA:
                        case BiomeType::SNOWY_TAIGA:
                            surfaceBlock = BlockType::SNOW;
                            break;
                        case BiomeType::RIVER:
                            surfaceBlock = BlockType::SAND; // River bed
                            break;
                        case BiomeType::SWAMP:
                            surfaceBlock = BlockType::DIRT; // Muddy swamp surface
                            break;
                        default:
                            // Most biomes use grass surface
                            surfaceBlock = BlockType::GRASS;
                            break;
                    }
                    
                    m_blocks[x][y][z].SetType(surfaceBlock);
                } else if (y >= terrainHeight - 3) {
                    // Dirt layer (3 blocks deep)
                    m_blocks[x][y][z].SetType(BlockType::DIRT);
                } else {
                    // Stone layer below
                    m_blocks[x][y][z].SetType(BlockType::STONE);
                }
            }
        }
    }
    
    // Generate water bodies (fill areas below sea level with water)
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int y = 0; y < SEA_LEVEL; ++y) {
                // If there's air below sea level, fill with water
                if (m_blocks[x][y][z].GetType() == BlockType::AIR) {
                    m_blocks[x][y][z].SetType(BlockType::WATER_STILL);
                }
            }
        }
    }
    
    // Generate trees in forest biomes
    for (int x = 2; x < CHUNK_WIDTH - 2; x += 3) {  // Space trees apart
        for (int z = 2; z < CHUNK_DEPTH - 2; z += 3) {
            // Convert local chunk coordinates to world coordinates
            int worldX = m_chunkX * CHUNK_WIDTH + x;
            int worldZ = m_chunkZ * CHUNK_DEPTH + z;
            
            // Get biome for this position
            BiomeType biomeType = BiomeSystem::GetBiomeType(worldX, worldZ, seed);
            
            // Generate trees in forest, taiga, jungle, and swamp biomes
            bool shouldGenerateTree = false;
            int treeChance = 7; // 70% chance by default
            
            switch (biomeType) {
                case BiomeType::FOREST:
                    shouldGenerateTree = true;
                    treeChance = 8;  // 80% chance in forests
                    break;
                case BiomeType::TAIGA:
                case BiomeType::SNOWY_TAIGA:
                    shouldGenerateTree = true;
                    treeChance = 6;  // 60% chance in taiga
                    break;
                case BiomeType::JUNGLE:
                    shouldGenerateTree = true;
                    treeChance = 9;  // 90% chance in jungle
                    break;
                case BiomeType::SWAMP:
                    shouldGenerateTree = true;
                    treeChance = 4;  // 40% chance in swamp
                    break;
                default:
                    shouldGenerateTree = false;
                    break;
            }
            
            if (shouldGenerateTree) {
                // Random chance for tree generation
                std::mt19937 treeRng(seed + worldX * 1000 + worldZ);
                if (static_cast<int>(treeRng() % 10) < treeChance) {
                    GenerateTree(x, z, treeRng, blockManager);
                }
            }
        }
    }
    
    // Mark mesh as needing regeneration
    m_meshGenerated = false;
}

void Chunk::ApplyServerData(const uint16_t* blockData) {
    // Clear existing blocks
    Clear();
    
    // Deserialize block data from server
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                int index = x + (y * 16) + (z * 16 * 256);
                BlockType blockType = static_cast<BlockType>(blockData[index]);
                m_blocks[x][y][z].SetType(blockType);
            }
        }
    }
    
    // Mark mesh as needing regeneration
    m_meshGenerated = false;
    
    std::cout << "Applied server data to chunk (" << m_chunkX << ", " << m_chunkZ << ")" << std::endl;
}

// Perlin noise implementation
double Chunk::Perlin(double x, double z, int seed) const {
    // Simple 2D Perlin noise implementation
    std::hash<int> hasher;
    
    // Get integer and fractional parts
    int xi = static_cast<int>(std::floor(x)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;
    double xf = x - std::floor(x);
    double zf = z - std::floor(z);
    
    // Fade curves
    double u = Fade(xf);
    double v = Fade(zf);
    
    // Hash coordinates with seed
    int aa = hasher(xi + hasher(zi + seed));
    int ab = hasher(xi + hasher(zi + 1 + seed));
    int ba = hasher(xi + 1 + hasher(zi + seed));
    int bb = hasher(xi + 1 + hasher(zi + 1 + seed));
    
    // Interpolate
    double x1 = Lerp(u, Grad(aa, xf, zf), Grad(ba, xf - 1, zf));
    double x2 = Lerp(u, Grad(ab, xf, zf - 1), Grad(bb, xf - 1, zf - 1));
    
    return Lerp(v, x1, x2);
}

double Chunk::Fade(double t) const {
    // Smooth fade function: 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Chunk::Lerp(double t, double a, double b) const {
    return a + t * (b - a);
}

double Chunk::Grad(int hash, double x, double z) const {
    // Simple gradient function for 2D noise
    int h = hash & 3;
    double u = h < 2 ? x : z;
    double v = h < 2 ? z : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
} 

// Helper method to get a block at an arbitrary offset from the current position
// Handles cross-chunk boundaries properly
Block Chunk::GetBlockAtOffset(int x, int y, int z, int dx, int dy, int dz, const World* world) const {
    int targetX = x + dx;
    int targetY = y + dy;
    int targetZ = z + dz;
    
    // If target is within this chunk, get it directly
    if (IsValidPosition(targetX, targetY, targetZ)) {
        return m_blocks[targetX][targetY][targetZ];
    }
    
    // Target is outside this chunk - convert to world coordinates and query world
    if (world) {
        int worldX = m_chunkX * CHUNK_WIDTH + targetX;
        int worldY = targetY;
        int worldZ = m_chunkZ * CHUNK_DEPTH + targetZ;
        
        return world->GetBlock(worldX, worldY, worldZ);
    }
    
    // No world access - assume air
    return Block(BlockType::AIR);
}

// Calculate ambient occlusion for a specific vertex of a face
// Simplified version - just samples blocks that would block ambient light to this vertex
float Chunk::CalculateVertexAO(int x, int y, int z, int faceDirection, int vertexIndex, const World* world) const {
    // Key insight: we need to sample blocks that are adjacent to where the vertex will be positioned
    // For a TOP face, vertices are on the top surface, so we sample blocks ABOVE that position
    // For a FRONT face, vertices are on the front surface, so we sample blocks IN FRONT of that position
    
    int side1_dx = 0, side1_dy = 0, side1_dz = 0;
    int side2_dx = 0, side2_dy = 0, side2_dz = 0;
    int corner_dx = 0, corner_dy = 0, corner_dz = 0;
    
    // The sampling pattern depends on the face and vertex position
    switch (faceDirection) {
        case FACE_TOP: { // Top face - sample blocks above
            // All samples are 1 block above the current block
            switch (vertexIndex) {
                case 0: // (-0.5, +0.5, -0.5) vertex
                    side1_dx = -1; side1_dy = 1; side1_dz = 0;   // Left-above
                    side2_dx = 0; side2_dy = 1; side2_dz = -1;   // Front-above
                    corner_dx = -1; corner_dy = 1; corner_dz = -1; // Corner-above
                    break;
                case 1: // (+0.5, +0.5, -0.5) vertex
                    side1_dx = 1; side1_dy = 1; side1_dz = 0;    // Right-above
                    side2_dx = 0; side2_dy = 1; side2_dz = -1;   // Front-above
                    corner_dx = 1; corner_dy = 1; corner_dz = -1; // Corner-above
                    break;
                case 2: // (+0.5, +0.5, +0.5) vertex
                    side1_dx = 1; side1_dy = 1; side1_dz = 0;    // Right-above
                    side2_dx = 0; side2_dy = 1; side2_dz = 1;    // Back-above
                    corner_dx = 1; corner_dy = 1; corner_dz = 1;  // Corner-above
                    break;
                case 3: // (-0.5, +0.5, +0.5) vertex
                    side1_dx = -1; side1_dy = 1; side1_dz = 0;   // Left-above
                    side2_dx = 0; side2_dy = 1; side2_dz = 1;    // Back-above
                    corner_dx = -1; corner_dy = 1; corner_dz = 1; // Corner-above
                    break;
            }
            break;
        }
        case FACE_BOTTOM: { // Bottom face - sample blocks below
            switch (vertexIndex) {
                case 0: 
                    side1_dx = -1; side1_dy = -1; side1_dz = 0;
                    side2_dx = 0; side2_dy = -1; side2_dz = -1;
                    corner_dx = -1; corner_dy = -1; corner_dz = -1;
                    break;
                case 1:
                    side1_dx = 1; side1_dy = -1; side1_dz = 0;
                    side2_dx = 0; side2_dy = -1; side2_dz = -1;
                    corner_dx = 1; corner_dy = -1; corner_dz = -1;
                    break;
                case 2:
                    side1_dx = 1; side1_dy = -1; side1_dz = 0;
                    side2_dx = 0; side2_dy = -1; side2_dz = 1;
                    corner_dx = 1; corner_dy = -1; corner_dz = 1;
                    break;
                case 3:
                    side1_dx = -1; side1_dy = -1; side1_dz = 0;
                    side2_dx = 0; side2_dy = -1; side2_dz = 1;
                    corner_dx = -1; corner_dy = -1; corner_dz = 1;
                    break;
            }
            break;
        }
        case FACE_FRONT: { // Front face - sample blocks in front (+Z)
            switch (vertexIndex) {
                case 0:
                    side1_dx = -1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 0; side2_dy = -1; side2_dz = 1;
                    corner_dx = -1; corner_dy = -1; corner_dz = 1;
                    break;
                case 1:
                    side1_dx = 1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 0; side2_dy = -1; side2_dz = 1;
                    corner_dx = 1; corner_dy = -1; corner_dz = 1;
                    break;
                case 2:
                    side1_dx = 1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 0; side2_dy = 1; side2_dz = 1;
                    corner_dx = 1; corner_dy = 1; corner_dz = 1;
                    break;
                case 3:
                    side1_dx = -1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 0; side2_dy = 1; side2_dz = 1;
                    corner_dx = -1; corner_dy = 1; corner_dz = 1;
                    break;
            }
            break;
        }
        case FACE_BACK: { // Back face - sample blocks behind (-Z)
            switch (vertexIndex) {
                case 0:
                    side1_dx = -1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 0; side2_dy = -1; side2_dz = -1;
                    corner_dx = -1; corner_dy = -1; corner_dz = -1;
                    break;
                case 1:
                    side1_dx = 1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 0; side2_dy = -1; side2_dz = -1;
                    corner_dx = 1; corner_dy = -1; corner_dz = -1;
                    break;
                case 2:
                    side1_dx = 1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 0; side2_dy = 1; side2_dz = -1;
                    corner_dx = 1; corner_dy = 1; corner_dz = -1;
                    break;
                case 3:
                    side1_dx = -1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 0; side2_dy = 1; side2_dz = -1;
                    corner_dx = -1; corner_dy = 1; corner_dz = -1;
                    break;
            }
            break;
        }
        case FACE_LEFT: { // Left face - sample blocks to the left (-X)
            switch (vertexIndex) {
                case 0:
                    side1_dx = -1; side1_dy = 0; side1_dz = -1;
                    side2_dx = -1; side2_dy = -1; side2_dz = 0;
                    corner_dx = -1; corner_dy = -1; corner_dz = -1;
                    break;
                case 1:
                    side1_dx = -1; side1_dy = 0; side1_dz = 1;
                    side2_dx = -1; side2_dy = -1; side2_dz = 0;
                    corner_dx = -1; corner_dy = -1; corner_dz = 1;
                    break;
                case 2:
                    side1_dx = -1; side1_dy = 0; side1_dz = 1;
                    side2_dx = -1; side2_dy = 1; side2_dz = 0;
                    corner_dx = -1; corner_dy = 1; corner_dz = 1;
                    break;
                case 3:
                    side1_dx = -1; side1_dy = 0; side1_dz = -1;
                    side2_dx = -1; side2_dy = 1; side2_dz = 0;
                    corner_dx = -1; corner_dy = 1; corner_dz = -1;
                    break;
            }
            break;
        }
        case FACE_RIGHT: { // Right face - sample blocks to the right (+X)
            switch (vertexIndex) {
                case 0:
                    side1_dx = 1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 1; side2_dy = -1; side2_dz = 0;
                    corner_dx = 1; corner_dy = -1; corner_dz = -1;
                    break;
                case 1:
                    side1_dx = 1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 1; side2_dy = -1; side2_dz = 0;
                    corner_dx = 1; corner_dy = -1; corner_dz = 1;
                    break;
                case 2:
                    side1_dx = 1; side1_dy = 0; side1_dz = 1;
                    side2_dx = 1; side2_dy = 1; side2_dz = 0;
                    corner_dx = 1; corner_dy = 1; corner_dz = 1;
                    break;
                case 3:
                    side1_dx = 1; side1_dy = 0; side1_dz = -1;
                    side2_dx = 1; side2_dy = 1; side2_dz = 0;
                    corner_dx = 1; corner_dy = 1; corner_dz = -1;
                    break;
            }
            break;
        }
    }
    
    // Sample the 3 neighbor blocks
    Block side1 = GetBlockAtOffset(x, y, z, side1_dx, side1_dy, side1_dz, world);
    Block side2 = GetBlockAtOffset(x, y, z, side2_dx, side2_dy, side2_dz, world);
    Block corner = GetBlockAtOffset(x, y, z, corner_dx, corner_dy, corner_dz, world);
    
    // Convert to boolean (solid = true, air = false)
    bool s1 = !side1.IsAir();
    bool s2 = !side2.IsAir();
    bool c = !corner.IsAir();
    
    // Apply Minecraft's ambient occlusion formula
    if (s1 && s2) {
        return 0.25f;  // Fully occluded - but not completely black
    }
    
    // Count number of occluding blocks and get base AO value
    int occluded = (s1 ? 1 : 0) + (s2 ? 1 : 0) + (c ? 1 : 0);
    float baseAO;
    switch (occluded) {
        case 0: baseAO = 1.0f; break;   // No occlusion - full brightness
        case 1: baseAO = 0.8f; break;   // Light occlusion
        case 2: baseAO = 0.6f; break;   // Medium occlusion  
        case 3: baseAO = 0.4f; break;   // Heavy occlusion
        default: baseAO = 1.0f; break;
    }
    
    // Apply Minecraft-style directional face multipliers
    float faceMultiplier;
    switch (faceDirection) {
        case FACE_TOP:    faceMultiplier = 1.0f; break;  // 100% - brightest (sky exposure)
        case FACE_FRONT:  // North face
        case FACE_BACK:   faceMultiplier = 0.8f; break;  // 80% - N/S faces  
        case FACE_LEFT:   // West face
        case FACE_RIGHT:  faceMultiplier = 0.6f; break;  // 60% - E/W faces
        case FACE_BOTTOM: faceMultiplier = 0.5f; break;  // 50% - darkest (no sky)
        default: faceMultiplier = 1.0f; break;
    }
    
    return baseAO * faceMultiplier;
}

void Chunk::GenerateTree(int x, int z, std::mt19937& rng, const BlockManager* blockManager) {
    // Find the surface height at this position
    int surfaceY = -1;
    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
        if (IsValidPosition(x, y, z) && m_blocks[x][y][z].GetType() == BlockType::GRASS) {
            surfaceY = y;
            break;
        }
    }
    
    if (surfaceY == -1) return;  // No grass surface found
    
    // Choose tree type (50% oak, 50% birch)
    bool isOak = (rng() % 2 == 0);
    
    if (isOak) {
        GenerateOakTree(x, z, surfaceY, rng, blockManager);
    } else {
        GenerateBirchTree(x, z, surfaceY, rng, blockManager);
    }
}

void Chunk::GenerateOakTree(int x, int z, int surfaceY, std::mt19937& rng, const BlockManager* blockManager) {
    // Oak tree: 4-6 blocks tall trunk with dense, rounded canopy
    int trunkHeight = 4 + (rng() % 3);  // 4-6 blocks tall
    
    // Generate trunk
    for (int y = 1; y <= trunkHeight; y++) {
        int trunkY = surfaceY + y;
        if (trunkY < CHUNK_HEIGHT && IsValidPosition(x, trunkY, z)) {
            BlockType oakLogType = blockManager ? blockManager->GetBlockTypeByKey("oak_log") : BlockType::AIR;
            m_blocks[x][trunkY][z].SetType(oakLogType);
        }
    }
    
    // Generate oak leaves (dense, rounded canopy)
    int leafStart = surfaceY + trunkHeight - 1;  // Start leaves 1 block below top of trunk
    
    // Layer 1: Bottom layer of leaves (3x3)
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            int leafX = x + dx;
            int leafY = leafStart;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // Skip corners for more natural look
                if (abs(dx) == 2 && abs(dz) == 2) {
                    if (rng() % 3 == 0) continue;  // 66% chance to skip corners
                }
                
                if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                    BlockType oakLeavesType = blockManager ? blockManager->GetBlockTypeByKey("oak_leaves") : BlockType::AIR;
                    m_blocks[leafX][leafY][leafZ].SetType(oakLeavesType);
                }
            }
        }
    }
    
    // Layer 2: Middle layer (3x3, denser)
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            int leafX = x + dx;
            int leafY = leafStart + 1;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // More selective on edges
                if (abs(dx) == 2 || abs(dz) == 2) {
                    if (rng() % 2 == 0) continue;  // 50% chance for edges
                }
                
                if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                    BlockType oakLeavesType = blockManager ? blockManager->GetBlockTypeByKey("oak_leaves") : BlockType::AIR;
                    m_blocks[leafX][leafY][leafZ].SetType(oakLeavesType);
                }
            }
        }
    }
    
    // Layer 3: Top layer (smaller, 1x1 plus some neighbors)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int leafX = x + dx;
            int leafY = leafStart + 2;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // Center and adjacent blocks
                if (dx == 0 && dz == 0) {
                    // Always place center
                                    if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                    BlockType oakLeavesType = blockManager ? blockManager->GetBlockTypeByKey("oak_leaves") : BlockType::AIR;
                    m_blocks[leafX][leafY][leafZ].SetType(oakLeavesType);
                }
                } else if (abs(dx) + abs(dz) == 1) {
                    // 75% chance for adjacent blocks
                                    if (rng() % 4 != 0 && m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                    BlockType oakLeavesType = blockManager ? blockManager->GetBlockTypeByKey("oak_leaves") : BlockType::AIR;
                    m_blocks[leafX][leafY][leafZ].SetType(oakLeavesType);
                }
                }
            }
        }
    }
}

void Chunk::GenerateBirchTree(int x, int z, int surfaceY, std::mt19937& rng, const BlockManager* blockManager) {
    // Birch tree: taller (5-7 blocks), thinner canopy
    int trunkHeight = 5 + (rng() % 3);  // 5-7 blocks tall
    
    // Generate trunk
    for (int y = 1; y <= trunkHeight; y++) {
        int trunkY = surfaceY + y;
        if (trunkY < CHUNK_HEIGHT && IsValidPosition(x, trunkY, z)) {
            BlockType birchLogType = blockManager ? blockManager->GetBlockTypeByKey("birch_log") : BlockType::AIR;
            m_blocks[x][trunkY][z].SetType(birchLogType);
        }
    }
    
    // Generate birch leaves (more sparse, taller canopy)
    int leafStart = surfaceY + trunkHeight - 2;  // Start leaves 2 blocks below top
    
    // Layer 1: Bottom layer (small, 1x1 plus cross)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int leafX = x + dx;
            int leafY = leafStart;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // Only center and cross pattern
                if ((dx == 0 && dz == 0) || (abs(dx) + abs(dz) == 1)) {
                    if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                        BlockType birchLeavesType = blockManager ? blockManager->GetBlockTypeByKey("birch_leaves") : BlockType::AIR;
                        m_blocks[leafX][leafY][leafZ].SetType(birchLeavesType);
                    }
                }
            }
        }
    }
    
    // Layer 2: Middle layer (2x2 around center)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int leafX = x + dx;
            int leafY = leafStart + 1;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // Skip corners sometimes for natural look
                if (abs(dx) == 1 && abs(dz) == 1) {
                    if (rng() % 3 == 0) continue;
                }
                
                if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                    BlockType birchLeavesType = blockManager ? blockManager->GetBlockTypeByKey("birch_leaves") : BlockType::AIR;
                    m_blocks[leafX][leafY][leafZ].SetType(birchLeavesType);
                }
            }
        }
    }
    
    // Layer 3: Top layer (cross pattern)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int leafX = x + dx;
            int leafY = leafStart + 2;
            int leafZ = z + dz;
            
            if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
                // Center and cross only
                if ((dx == 0 && dz == 0) || (abs(dx) + abs(dz) == 1)) {
                    if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                        BlockType birchLeavesType = blockManager ? blockManager->GetBlockTypeByKey("birch_leaves") : BlockType::AIR;
                        m_blocks[leafX][leafY][leafZ].SetType(birchLeavesType);
                    }
                }
            }
        }
    }
    
    // Layer 4: Very top (just center, sometimes)
    int leafX = x;
    int leafY = leafStart + 3;
    int leafZ = z;
    
    if (IsValidPosition(leafX, leafY, leafZ) && leafY < CHUNK_HEIGHT) {
        if (rng() % 2 == 0) {  // 50% chance for top leaf
            if (m_blocks[leafX][leafY][leafZ].GetType() == BlockType::AIR) {
                BlockType birchLeavesType = blockManager ? blockManager->GetBlockTypeByKey("birch_leaves") : BlockType::AIR;
                m_blocks[leafX][leafY][leafZ].SetType(birchLeavesType);
            }
        }
    }
}  