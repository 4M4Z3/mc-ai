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
    void EndFrame();
    
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
    
    // Texture management
    std::vector<unsigned int> m_blockTextures;
    unsigned int LoadTexture(const std::string& filepath);
    bool LoadBlockTextures();
    
    // Projection matrix
    Mat4 m_projectionMatrix;
    int m_viewportWidth, m_viewportHeight;
    
    // Shader uniforms
    int m_modelLoc, m_viewLoc, m_projLoc;
    int m_textureLoc;

    // Shader management
    bool CreateShaders();
    bool CreateCubeGeometry();
    std::string LoadShaderSource(const std::string& filepath);
    unsigned int CompileShader(unsigned int type, const char* source);
    bool CheckShaderCompilation(unsigned int shader, const char* type);
    bool CheckProgramLinking(unsigned int program);
    
    // Matrix operations
    Mat4 CreateProjectionMatrix(float fov, float aspect, float near, float far);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
}; 