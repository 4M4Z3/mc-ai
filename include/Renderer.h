#pragma once

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <epoxy/gl.h>
#endif
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vector>
#include <string>
#include "Player.h"
#include "Server.h" // For PlayerPosition
#include "Block.h"
#include "BlockManager.h"
#include "PlayerModel.h"

class World;

// Frustum culling structures
struct Plane {
    Vec3 normal;
    float distance;
    
    Plane(const Vec3& n, float d) : normal(n), distance(d) {}
    
    // Distance from point to plane (positive = in front, negative = behind)
    float DistanceToPoint(const Vec3& point) const {
        return normal.x * point.x + normal.y * point.y + normal.z * point.z + distance;
    }
};

struct Frustum {
    Plane planes[6]; // Left, Right, Bottom, Top, Near, Far
    
    Frustum() : planes{
        Plane(Vec3(0,0,0), 0), Plane(Vec3(0,0,0), 0), Plane(Vec3(0,0,0), 0),
        Plane(Vec3(0,0,0), 0), Plane(Vec3(0,0,0), 0), Plane(Vec3(0,0,0), 0)
    } {}
};

struct AABB {
    Vec3 min, max;
    
    AABB(const Vec3& minPoint, const Vec3& maxPoint) : min(minPoint), max(maxPoint) {}
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();
    void Clear();
    void SetViewport(int width, int height);
    
    // 3D rendering
    void BeginFrame(const Player& player);
    void RenderWorld(const World& world);
    void RenderChunks(const World& world);  // New chunk-based rendering
    void RenderCube(float x, float y, float z);  // Legacy individual cube rendering
    void RenderOtherPlayers(const std::vector<PlayerPosition>& playerPositions);
    void RenderSky(float gameTime); // Render sky with sun/moon based on time
    void EndFrame();
    
    // Block targeting wireframe
    void RenderBlockWireframe(const Vec3& blockPos, const World& world);
    
    // Legacy triangle rendering (for debugging)
    void RenderTriangle();

    // Frustum culling methods
    void ExtractFrustum(const Mat4& viewMatrix, const Mat4& projMatrix);
    bool IsChunkInFrustum(int chunkX, int chunkZ) const;
    bool IsAABBInFrustum(const AABB& aabb) const;
    
    // Debug flag to disable frustum culling temporarily
    // Starting with culling disabled until we verify it works correctly with 70° FOV
    bool m_enableFrustumCulling = false;

    // Texture access for UI rendering
    unsigned int GetHotbarTexture() const { return m_hotbarTexture; }
    unsigned int GetInventoryTexture() const { return m_inventoryTexture; }
    
    // Item texture management
    unsigned int LoadItemTexture(const std::string& itemIconPath);
    unsigned int GetItemTexture(const std::string& itemIconPath);
    unsigned int GetHotbarSelectionTexture() const { return m_hotbarSelectionTexture; }

    // Block management (public for world generation access)
    BlockManager m_blockManager;
    
private:
    // Cube rendering data (legacy)
    unsigned int m_cubeVAO;
    unsigned int m_cubeVBO;
    unsigned int m_shaderProgram;
    
    // Triangle data (legacy)
    unsigned int m_triangleVAO;
    unsigned int m_triangleVBO;
    
    // Wireframe rendering for block targeting
    unsigned int m_wireframeVAO;
    unsigned int m_wireframeVBO;
    unsigned int m_wireframeShaderProgram;
    int m_wireframeModelLoc, m_wireframeViewLoc, m_wireframeProjLoc;
    
    // Player model rendering
    PlayerModel m_playerModel;
    unsigned int m_playerShaderProgram;
    int m_playerModelLoc, m_playerViewLoc, m_playerProjLoc;
    
    // Sky rendering
    unsigned int m_skyVAO;
    unsigned int m_skyVBO;
    unsigned int m_skyShaderProgram;
    int m_skyViewLoc, m_skyProjLoc, m_skyGameTimeLoc, m_skySunDirLoc;
    
    // Texture management
    std::unordered_map<BlockType, unsigned int> m_blockTextures;
    
    // Special grass block textures (different per face)
    unsigned int m_grassTopTexture;
    unsigned int m_grassSideTexture;
    unsigned int m_grassSideOverlayTexture;
    unsigned int m_grassBottomTexture;
    
    // Log block textures (different per face, no overlay)
    unsigned int m_oakLogTopTexture;
    unsigned int m_oakLogSideTexture;
    unsigned int m_birchLogTopTexture;
    unsigned int m_birchLogSideTexture;
    unsigned int m_darkOakLogTopTexture;
    unsigned int m_darkOakLogSideTexture;
    
    // Sky textures
    unsigned int m_sunTexture;
    unsigned int m_moonTexture;
    
    // UI textures
    unsigned int m_hotbarTexture;
    unsigned int m_hotbarSelectionTexture;
    unsigned int m_inventoryTexture;
    
    // Item textures cache
    std::unordered_map<std::string, unsigned int> m_itemTextures;
    
    unsigned int LoadTexture(const std::string& filepath);
    unsigned int LoadTextureWithAlpha(const std::string& filepath);
    bool LoadBlockTextures();
    bool LoadSkyTextures();
    bool LoadHotbarTextures();
    bool LoadInventoryTextures();
    
    // Projection matrix
    Mat4 m_projectionMatrix;
    Mat4 m_viewMatrix;  // Store view matrix for player rendering
    int m_viewportWidth, m_viewportHeight;
    
    // Shader uniforms
    int m_modelLoc, m_viewLoc, m_projLoc;
    int m_textureLoc;
    int m_colorTintLoc;

    // Shader management
    bool CreateShaders();
    bool CreatePlayerShaders();
    bool CreateWireframeShaders();
    bool CreateSkyShaders();
    bool CreateCubeGeometry();
    bool CreateWireframeGeometry();
    bool CreateSkyGeometry();
    std::string LoadShaderSource(const std::string& filepath);
    unsigned int CompileShader(unsigned int type, const char* source);
    bool CheckShaderCompilation(unsigned int shader, const char* type);
    bool CheckProgramLinking(unsigned int program);
    
    // Biome-based tinting helpers
    bool NeedsBiomeTinting(BlockType blockType) const;
    bool IsLeafBlock(BlockType blockType) const;
    void ApplyBiomeTinting(BlockType blockType, int chunkX, int chunkZ, int worldSeed);
    
    // Matrix operations
    Mat4 CreateProjectionMatrix(float fov, float aspect, float near, float far);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
    
    // Frustum culling
    Frustum m_frustum;
}; 