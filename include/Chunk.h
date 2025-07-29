#pragma once

#include "Block.h"
#include <array>
#include <random>
#include <vector>
#include <unordered_map> // Added for unordered_map

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <epoxy/gl.h>
#endif

// Forward declaration
class World;

// Chunk dimensions
constexpr int CHUNK_WIDTH = 16;
constexpr int CHUNK_HEIGHT = 256;
constexpr int CHUNK_DEPTH = 16;

class Chunk {
public:
    Chunk();
    Chunk(int chunkX, int chunkZ);
    ~Chunk();
    
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
    
    // Mesh rendering
    void GenerateMesh(const World* world);
    void RenderMesh() const;
    void RenderMeshForBlockType(BlockType blockType) const;
    std::vector<BlockType> GetBlockTypesInChunk() const;
    bool HasMesh() const { return !m_blockMeshes.empty(); }
    void ClearMesh();

private:
    // 3D array of blocks [x][y][z]
    std::array<std::array<std::array<Block, CHUNK_DEPTH>, CHUNK_HEIGHT>, CHUNK_WIDTH> m_blocks;
    
    int m_chunkX;
    int m_chunkZ;
    
    // Mesh data per block type
    struct BlockMesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        int vertexCount = 0;
    };
    std::unordered_map<BlockType, BlockMesh> m_blockMeshes;
    bool m_meshGenerated;
    
    // Face culling helpers
    bool ShouldRenderFace(int x, int y, int z, int faceDirection, const World* world) const;
    Block GetNeighborBlock(int x, int y, int z, int faceDirection, const World* world) const;
    void AddFaceToMesh(std::vector<float>& vertices, int x, int y, int z, int faceDirection, const World* world) const;
    
    // Ambient occlusion calculation
    float CalculateVertexAO(int x, int y, int z, int faceDirection, int vertexIndex, const World* world) const;
    Block GetBlockAtOffset(int x, int y, int z, int dx, int dy, int dz, const World* world) const;
    
    // Face direction constants
    enum Face {
        FACE_FRONT = 0,   // +Z
        FACE_BACK = 1,    // -Z  
        FACE_LEFT = 2,    // -X
        FACE_RIGHT = 3,   // +X
        FACE_BOTTOM = 4,  // -Y
        FACE_TOP = 5      // +Y
    };
    
    // Perlin noise utilities for world generation
    double Perlin(double x, double z, int seed) const;
    double Fade(double t) const;
    double Lerp(double t, double a, double b) const;
    double Grad(int hash, double x, double z) const;
    
    // Terrain generation parameters
    static constexpr double NOISE_SCALE = 0.05;  // Controls hill frequency
    static constexpr int BASE_HEIGHT = 64;       // Base terrain height
    static constexpr int MAX_HEIGHT_VARIATION = 16; // Maximum hill height above base
}; 