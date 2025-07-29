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
#include "PlayerModel.h"

class World;

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
    void RenderOtherPlayers(const std::unordered_map<uint32_t, PlayerPosition>& otherPlayers);
    void RenderSky(float gameTime); // Render sky with sun/moon based on time
    void EndFrame();
    
    // Block targeting wireframe
    void RenderBlockWireframe(const Vec3& blockPos, const World& world);
    
    // Legacy triangle rendering (for debugging)
    void RenderTriangle();

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
    
    // Texture management
    std::vector<unsigned int> m_blockTextures;
    
    // Special grass block textures (different per face)
    unsigned int m_grassTopTexture;
    unsigned int m_grassSideTexture;
    unsigned int m_grassSideOverlayTexture;
    unsigned int m_grassBottomTexture;
    
    // Sky textures
    unsigned int m_sunTexture;
    unsigned int m_moonTexture;
    
    unsigned int LoadTexture(const std::string& filepath);
    unsigned int LoadTextureWithAlpha(const std::string& filepath);
    bool LoadBlockTextures();
    bool LoadSkyTextures();
    
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
    bool CreateCubeGeometry();
    bool CreateWireframeGeometry();
    std::string LoadShaderSource(const std::string& filepath);
    unsigned int CompileShader(unsigned int type, const char* source);
    bool CheckShaderCompilation(unsigned int shader, const char* type);
    bool CheckProgramLinking(unsigned int program);
    
    // Matrix operations
    Mat4 CreateProjectionMatrix(float fov, float aspect, float near, float far);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
}; 