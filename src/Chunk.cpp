#include "Chunk.h"
#include "World.h"
#include <cmath>
#include <iostream>

Chunk::Chunk() : m_chunkX(0), m_chunkZ(0), m_VAO(0), m_VBO(0), m_vertexCount(0), m_meshGenerated(false) {
    Clear();
}

Chunk::Chunk(int chunkX, int chunkZ) : m_chunkX(chunkX), m_chunkZ(chunkZ), m_VAO(0), m_VBO(0), m_vertexCount(0), m_meshGenerated(false) {
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
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    m_vertexCount = 0;
    m_meshGenerated = false;
}

void Chunk::GenerateMesh(const World* world) {
    // Clear existing mesh
    ClearMesh();
    
    std::vector<float> vertices;
    vertices.reserve(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH * 6 * 6 * 3); // Worst case: all faces visible
    
    // Generate mesh for all non-air blocks
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_DEPTH; ++z) {
                if (!m_blocks[x][y][z].IsAir()) {
                    // Check each face for visibility
                    for (int face = 0; face < 6; ++face) {
                        if (ShouldRenderFace(x, y, z, face, world)) {
                            AddFaceToMesh(vertices, x, y, z, face, world);
                        }
                    }
                }
            }
        }
    }
    
    // If no vertices, don't create mesh
    if (vertices.empty()) {
        m_vertexCount = 0;
        m_meshGenerated = true;
        return;
    }
    
    // Create OpenGL mesh
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Ambient occlusion attribute (ao)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    m_vertexCount = vertices.size() / 4;
    m_meshGenerated = true;
}

void Chunk::RenderMesh() const {
    if (!HasMesh() || m_VAO == 0) {
        return;
    }
    
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);
}

bool Chunk::ShouldRenderFace(int x, int y, int z, int faceDirection, const World* world) const {
    Block neighbor = GetNeighborBlock(x, y, z, faceDirection, world);
    return neighbor.IsAir();
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

void Chunk::AddFaceToMesh(std::vector<float>& vertices, int x, int y, int z, int faceDirection, const World* world) const {
    // Convert local chunk coordinates to world position for rendering
    float worldX = static_cast<float>(m_chunkX * CHUNK_WIDTH + x);
    float worldY = static_cast<float>(y);
    float worldZ = static_cast<float>(m_chunkZ * CHUNK_DEPTH + z);
    
    // Face vertices (relative to block center) - now includes AO values
    // Each vertex: x, y, z, ao_value (4 floats per vertex)
    
    switch (faceDirection) {
        case FACE_FRONT: { // +Z face
            // Calculate AO for each vertex of the front face
            float ao0 = CalculateVertexAO(x, y, z, FACE_FRONT, 0, world); // Bottom-left
            float ao1 = CalculateVertexAO(x, y, z, FACE_FRONT, 1, world); // Bottom-right  
            float ao2 = CalculateVertexAO(x, y, z, FACE_FRONT, 2, world); // Top-right
            float ao3 = CalculateVertexAO(x, y, z, FACE_FRONT, 3, world); // Top-left
            
            float frontVertices[] = {
                // Triangle 1
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao0, // Bottom-left
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1, // Bottom-right
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, // Top-right
                // Triangle 2  
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2, // Top-right
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao3, // Top-left
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao0  // Bottom-left
            };
            vertices.insert(vertices.end(), frontVertices, frontVertices + 24);
            break;
        }
        case FACE_BACK: { // -Z face
            float ao0 = CalculateVertexAO(x, y, z, FACE_BACK, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_BACK, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_BACK, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_BACK, 3, world);
            
            float backVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0,
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao2,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao2,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao1,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0
            };
            vertices.insert(vertices.end(), backVertices, backVertices + 24);
            break;
        }
        case FACE_LEFT: { // -X face
            float ao0 = CalculateVertexAO(x, y, z, FACE_LEFT, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_LEFT, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_LEFT, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_LEFT, 3, world);
            
            float leftVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0,
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1,
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0
            };
            vertices.insert(vertices.end(), leftVertices, leftVertices + 24);
            break;
        }
        case FACE_RIGHT: { // +X face
            float ao0 = CalculateVertexAO(x, y, z, FACE_RIGHT, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_RIGHT, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_RIGHT, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_RIGHT, 3, world);
            
            float rightVertices[] = {
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao3,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao1,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0
            };
            vertices.insert(vertices.end(), rightVertices, rightVertices + 24);
            break;
        }
        case FACE_BOTTOM: { // -Y face
            float ao0 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_BOTTOM, 3, world);
            
            float bottomVertices[] = {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0,
                worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f, ao1,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao2,
                worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f, ao2,
                worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f, ao3,
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f, ao0
            };
            vertices.insert(vertices.end(), bottomVertices, bottomVertices + 24);
            break;
        }
        case FACE_TOP: { // +Y face
            float ao0 = CalculateVertexAO(x, y, z, FACE_TOP, 0, world);
            float ao1 = CalculateVertexAO(x, y, z, FACE_TOP, 1, world);
            float ao2 = CalculateVertexAO(x, y, z, FACE_TOP, 2, world);
            float ao3 = CalculateVertexAO(x, y, z, FACE_TOP, 3, world);
            
            float topVertices[] = {
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao0,
                worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f, ao3,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f, ao2,
                worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f, ao1,
                worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f, ao0
            };
            vertices.insert(vertices.end(), topVertices, topVertices + 24);
            break;
        }
    }
}

void Chunk::Generate(int seed) {
    // Fill all blocks with air first
    Clear();
    
    // Generate terrain using Perlin noise
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            // Convert local chunk coordinates to world coordinates
            int worldX = m_chunkX * CHUNK_WIDTH + x;
            int worldZ = m_chunkZ * CHUNK_DEPTH + z;
            
            // Get height from Perlin noise
            double noise = Perlin(worldX * NOISE_SCALE, worldZ * NOISE_SCALE, seed);
            
            // Map noise from [-1, 1] to [0, 1] and scale to height variation
            double normalizedNoise = (noise + 1.0) * 0.5;
            int terrainHeight = BASE_HEIGHT + static_cast<int>(normalizedNoise * MAX_HEIGHT_VARIATION);
            
            // Clamp height to valid range
            terrainHeight = std::max(0, std::min(terrainHeight, CHUNK_HEIGHT - 1));
            
            // Fill blocks from bottom up to terrain height
            for (int y = 0; y <= terrainHeight; ++y) {
                if (y == terrainHeight) {
                    // Top layer: grass/surface
                    m_blocks[x][y][z].SetType(BlockType::BLOCK);
                } else if (y >= terrainHeight - 3) {
                    // Dirt layer (3 blocks deep)
                    m_blocks[x][y][z].SetType(BlockType::BLOCK);
                } else {
                    // Stone layer below
                    m_blocks[x][y][z].SetType(BlockType::BLOCK);
                }
            }
        }
    }
    
    // Mark mesh as needing regeneration
    m_meshGenerated = false;
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