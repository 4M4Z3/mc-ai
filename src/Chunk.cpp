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
                            AddFaceToMesh(vertices, x, y, z, face);
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
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    m_vertexCount = vertices.size() / 3;
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

void Chunk::AddFaceToMesh(std::vector<float>& vertices, int x, int y, int z, int faceDirection) const {
    // Convert local chunk coordinates to world position for rendering
    float worldX = static_cast<float>(m_chunkX * CHUNK_WIDTH + x);
    float worldY = static_cast<float>(y);
    float worldZ = static_cast<float>(m_chunkZ * CHUNK_DEPTH + z);
    
    // Face vertices (relative to block center)
    float faceVertices[6][18]; // 6 faces, each with 6 vertices (2 triangles), each vertex has 3 coordinates
    
    // Front face (+Z)
    float frontFace[18] = {
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f
    };
    
    // Back face (-Z)
    float backFace[18] = {
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f
    };
    
    // Left face (-X)
    float leftFace[18] = {
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
    };
    
    // Right face (+X)
    float rightFace[18] = {
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f
    };
    
    // Bottom face (-Y)
    float bottomFace[18] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f
    };
    
    // Top face (+Y)
    float topFace[18] = {
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    // Copy face data into array
    for (int i = 0; i < 18; ++i) frontFace[i] = frontFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_FRONT][i] = frontFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_BACK][i] = backFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_LEFT][i] = leftFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_RIGHT][i] = rightFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_BOTTOM][i] = bottomFace[i];
    for (int i = 0; i < 18; ++i) faceVertices[FACE_TOP][i] = topFace[i];
    
    // Add the face vertices to the mesh, translated to world position
    for (int i = 0; i < 18; i += 3) {
        vertices.push_back(faceVertices[faceDirection][i] + worldX);       // X
        vertices.push_back(faceVertices[faceDirection][i + 1] + worldY);   // Y  
        vertices.push_back(faceVertices[faceDirection][i + 2] + worldZ);   // Z
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