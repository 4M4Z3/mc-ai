#include "Renderer.h"
#include "World.h"
#include "Chunk.h"
#include "PlayerModel.h"
#include "BiomeSystem.h"
#include "Debug.h"
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

Renderer::Renderer() : m_cubeVAO(0), m_cubeVBO(0), m_shaderProgram(0), 
                       m_triangleVAO(0), m_triangleVBO(0),
                       m_wireframeVAO(0), m_wireframeVBO(0), m_wireframeShaderProgram(0),
                       m_wireframeModelLoc(-1), m_wireframeViewLoc(-1), m_wireframeProjLoc(-1),
                       m_playerShaderProgram(0),
                       m_playerModelLoc(-1), m_playerViewLoc(-1), m_playerProjLoc(-1),
                       m_skyVAO(0), m_skyVBO(0), m_skyShaderProgram(0),
                       m_skyViewLoc(-1), m_skyProjLoc(-1), m_skyGameTimeLoc(-1), m_skySunDirLoc(-1),
                       m_sunTexture(0), m_moonTexture(0),
                       m_viewportWidth(1280), m_viewportHeight(720) {
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    // Create cube geometry
    if (!CreateCubeGeometry()) {
        std::cerr << "Failed to create cube geometry" << std::endl;
        return false;
    }
    
    // Create triangle geometry (legacy)
    float triangleVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &m_triangleVAO);
    glBindVertexArray(m_triangleVAO);

    glGenBuffers(1, &m_triangleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create shaders
    if (!CreateShaders()) {
        std::cerr << "Failed to create shaders" << std::endl;
        return false;
    }
    
    // Create player shaders
    if (!CreatePlayerShaders()) {
        std::cerr << "Failed to create player shaders" << std::endl;
        return false;
    }
    
    // Create wireframe shaders
    if (!CreateWireframeShaders()) {
        std::cerr << "Failed to create wireframe shaders" << std::endl;
        return false;
    }
    
    // Create wireframe geometry
    if (!CreateWireframeGeometry()) {
        std::cerr << "Failed to create wireframe geometry" << std::endl;
        return false;
    }
    
    // Create sky shaders
    if (!CreateSkyShaders()) {
        std::cerr << "Failed to create sky shaders" << std::endl;
        return false;
    }
    
    // Create sky geometry
    if (!CreateSkyGeometry()) {
        std::cerr << "Failed to create sky geometry" << std::endl;
        return false;
    }
    
    // Create water shaders
    if (!CreateWaterShaders()) {
        std::cerr << "Failed to create water shaders" << std::endl;
        return false;
    }

    // Get uniform locations for block shaders
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    m_cameraYLoc = glGetUniformLocation(m_shaderProgram, "cameraY");
    
    // Get uniform locations for player shaders
    m_playerModelLoc = glGetUniformLocation(m_playerShaderProgram, "model");
    m_playerViewLoc = glGetUniformLocation(m_playerShaderProgram, "view");
    m_playerProjLoc = glGetUniformLocation(m_playerShaderProgram, "projection");
    
    // Get uniform locations for wireframe shaders
    m_wireframeModelLoc = glGetUniformLocation(m_wireframeShaderProgram, "model");
    m_wireframeViewLoc = glGetUniformLocation(m_wireframeShaderProgram, "view");  
    m_wireframeProjLoc = glGetUniformLocation(m_wireframeShaderProgram, "projection");
    
    // Get uniform locations for sky shaders
    m_skyViewLoc = glGetUniformLocation(m_skyShaderProgram, "view");
    m_skyProjLoc = glGetUniformLocation(m_skyShaderProgram, "projection");
    m_skyGameTimeLoc = glGetUniformLocation(m_skyShaderProgram, "gameTime");
    m_skySunDirLoc = glGetUniformLocation(m_skyShaderProgram, "sunDirection");
    
    // Get uniform locations for water shaders
    m_waterModelLoc = glGetUniformLocation(m_waterShaderProgram, "model");
    m_waterViewLoc = glGetUniformLocation(m_waterShaderProgram, "view");
    m_waterProjLoc = glGetUniformLocation(m_waterShaderProgram, "projection");
    m_waterTimeLoc = glGetUniformLocation(m_waterShaderProgram, "time");
    m_waterGameTimeLoc = glGetUniformLocation(m_waterShaderProgram, "gameTime");
    m_waterCameraPosLoc = glGetUniformLocation(m_waterShaderProgram, "cameraPos");
    m_waterSunDirLoc = glGetUniformLocation(m_waterShaderProgram, "sunDirection");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable multisampling for antialiasing (reduce shimmering)
    glEnable(GL_MULTISAMPLE);
    
    // Enable backface culling to hide faces facing away from camera
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Counter-clockwise winding for front faces
    
    // Load block textures AFTER ambient occlusion shaders
    if (!LoadBlockTextures()) {
        std::cerr << "Failed to load block textures" << std::endl;
        return false;
    }
    
    // Load sky textures
    if (!LoadSkyTextures()) {
        std::cerr << "Failed to load sky textures" << std::endl;
        return false;
    }
    
    // Load hotbar textures
    if (!LoadHotbarTextures()) {
        std::cerr << "Failed to load hotbar textures" << std::endl;
        return false;
    }
    
    if (!LoadInventoryTextures()) {
        std::cerr << "Failed to load inventory textures" << std::endl;
        return false;
    }
    
    // Get texture uniform location after loading textures
    m_textureLoc = glGetUniformLocation(m_shaderProgram, "blockTexture");
    m_colorTintLoc = glGetUniformLocation(m_shaderProgram, "colorTint");
    
    // Initialize player model
    if (!m_playerModel.Initialize()) {
        std::cerr << "Failed to initialize player model" << std::endl;
        return false;
    }
    
    // Get skin texture uniform location
    int playerSkinTextureLoc = glGetUniformLocation(m_playerShaderProgram, "skinTexture");
    
    // Set up player model with shader program and uniform locations
    m_playerModel.UseShaderProgram(m_playerShaderProgram);
    m_playerModel.SetUniformLocations(m_playerModelLoc, m_playerViewLoc, m_playerProjLoc, playerSkinTextureLoc);
    
    // Set initial viewport and projection
    SetViewport(m_viewportWidth, m_viewportHeight);

    DEBUG_INFO("3D Renderer initialized successfully!");
    return true;
}

bool Renderer::CreateCubeGeometry() {
    // Cube vertices with position (3), AO (1), and texture coordinates (2)  
    // Format: x, y, z, ao, u, v
    float cubeVertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
         0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // top-left
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left

        // Back face
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // bottom-left
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f,  // top-left
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // top-right
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // top-right
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // bottom-right
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // bottom-left

        // Left face
        -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // top-left
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
        -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
        -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right

        // Right face
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // top-left
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f,  // top-right
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // top-left

        // Bottom face
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // top-left
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f,  // top-right
         0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
         0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // top-left

        // Top face
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // bottom-left
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // top-left
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,  // top-right  
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // bottom-right
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f   // bottom-left
    };

    glGenVertexArrays(1, &m_cubeVAO);
    glBindVertexArray(m_cubeVAO);

    glGenBuffers(1, &m_cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // AO attribute (location = 1)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Renderer::Shutdown() {
    if (m_cubeVAO) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        m_cubeVAO = 0;
    }
    if (m_cubeVBO) {
        glDeleteBuffers(1, &m_cubeVBO);
        m_cubeVBO = 0;
    }
    if (m_triangleVAO) {
        glDeleteVertexArrays(1, &m_triangleVAO);
        m_triangleVAO = 0;
    }
    if (m_triangleVBO) {
        glDeleteBuffers(1, &m_triangleVBO);
        m_triangleVBO = 0;
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_playerShaderProgram) {
        glDeleteProgram(m_playerShaderProgram);
        m_playerShaderProgram = 0;
    }
    
    // Clean up wireframe resources
    if (m_wireframeVAO) {
        glDeleteVertexArrays(1, &m_wireframeVAO);
        m_wireframeVAO = 0;
    }
    if (m_wireframeVBO) {
        glDeleteBuffers(1, &m_wireframeVBO);
        m_wireframeVBO = 0;
    }
    if (m_wireframeShaderProgram) {
        glDeleteProgram(m_wireframeShaderProgram);
        m_wireframeShaderProgram = 0;
    }
    
    // Clean up sky resources
    if (m_skyVAO) {
        glDeleteVertexArrays(1, &m_skyVAO);
        m_skyVAO = 0;
    }
    if (m_skyVBO) {
        glDeleteBuffers(1, &m_skyVBO);
        m_skyVBO = 0;
    }
    if (m_skyShaderProgram) {
        glDeleteProgram(m_skyShaderProgram);
        m_skyShaderProgram = 0;
    }
    
    // Clean up player model
    m_playerModel.Shutdown();
    
    // Clean up textures
    for (const auto& pair : m_blockTextures) {
        unsigned int texture = pair.second;
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    m_blockTextures.clear();
    
    // Clean up grass textures
    if (m_grassTopTexture != 0) {
        glDeleteTextures(1, &m_grassTopTexture);
        m_grassTopTexture = 0;
    }
    if (m_grassSideTexture != 0) {
        glDeleteTextures(1, &m_grassSideTexture);
        m_grassSideTexture = 0;
    }
    if (m_grassSideOverlayTexture != 0) {
        glDeleteTextures(1, &m_grassSideOverlayTexture);
        m_grassSideOverlayTexture = 0;
    }
    if (m_grassBottomTexture != 0) {
        glDeleteTextures(1, &m_grassBottomTexture);
        m_grassBottomTexture = 0;
    }
    
    // Clean up sky textures
    if (m_sunTexture != 0) {
        glDeleteTextures(1, &m_sunTexture);
        m_sunTexture = 0;
    }
    if (m_moonTexture != 0) {
        glDeleteTextures(1, &m_moonTexture);
        m_moonTexture = 0;
    }
}

void Renderer::Clear() {
    // Sky color and clearing will be handled in RenderSky()
    // Don't clear color buffer here to avoid overriding sky rendering
    
    // Just ensure depth testing is configured properly
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void Renderer::SetViewport(int width, int height) {
    DEBUG_INFO("Setting viewport to: " << width << "x" << height);
    
    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
    
    // Update projection matrix with default FOV (will be overridden in BeginFrame)
    float aspect = (float)width / (float)height;
    m_projectionMatrix = CreateProjectionMatrix(70.0f, aspect, 0.1f, 100.0f);
    
    DEBUG_INFO("Aspect ratio: " << aspect);
}

void Renderer::BeginFrame(const Player& player) {
    // Ensure viewport is correct (in case ImGui or other code changed it)
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    glUseProgram(m_shaderProgram);
    
    // Set view matrix and store it for player rendering
    m_viewMatrix = player.GetViewMatrix();
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, m_viewMatrix.m);
    
    // Update projection matrix with current player FOV (for sprinting effect)
    float aspect = (float)m_viewportWidth / (float)m_viewportHeight;
    m_projectionMatrix = CreateProjectionMatrix(player.GetCurrentFOV(), aspect, 0.1f, 100.0f);
    
    // Set projection matrix
    glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Store camera Y position for underwater effect and full position for water reflections
    Vec3 playerPos = player.GetPosition();
    m_cameraY = playerPos.y;
    m_cameraPos = playerPos;
    glUniform1f(m_cameraYLoc, m_cameraY);
    
    // Extract frustum planes for culling
    ExtractFrustum(m_viewMatrix, m_projectionMatrix);
}

void Renderer::RenderWorld(const World& world, float gameTime) {
    // Use optimized chunk-based rendering instead of individual cubes
    RenderChunks(world, gameTime);
}

void Renderer::RenderChunks(const World& world, float gameTime) {
    // Set identity model matrix since chunks handle their own world positioning
    Mat4 modelMatrix;  // Identity matrix
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
    
    // Enable texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_textureLoc, 0);
    
    // Get all available block types from the block manager
    std::vector<BlockType> blockTypesToRender = m_blockManager.GetAllBlockTypes();
    
    // Render each block type separately with its texture
    for (BlockType blockType : blockTypesToRender) {
        // Skip AIR blocks and water blocks (water has its own shader)
        if (blockType == BlockType::AIR || 
            blockType == BlockType::WATER_STILL || 
            blockType == BlockType::WATER_FLOW) {
            continue;
        }
        
        // Handle grass blocks specially (different textures per face)
        if (blockType == BlockType::GRASS) {
            // Render grass top faces with biome-based tint
            glBindTexture(GL_TEXTURE_2D, m_grassTopTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 5;
                    int chunkZ = z - 5;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        ApplyBiomeTinting(blockType, chunkX, chunkZ, world.GetSeed());
                        chunk->RenderGrassMesh(Chunk::GRASS_TOP);
                    }
                }
            }
            
            // Render grass side faces with base texture (no tint)
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
            glBindTexture(GL_TEXTURE_2D, m_grassSideTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 5;
                    int chunkZ = z - 5;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderGrassMesh(Chunk::GRASS_SIDE);
                    }
                }
            }
            
            // Render grass side overlay on top using polygon offset to avoid z-fighting
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f); // Pull overlay slightly toward camera
            glBindTexture(GL_TEXTURE_2D, m_grassSideOverlayTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 5;
                    int chunkZ = z - 5;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        ApplyBiomeTinting(blockType, chunkX, chunkZ, world.GetSeed());
                        chunk->RenderGrassMesh(Chunk::GRASS_SIDE);
                    }
                }
            }
            glDisable(GL_POLYGON_OFFSET_FILL);
            
            // Render grass bottom faces (no tint) 
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
            glBindTexture(GL_TEXTURE_2D, m_grassBottomTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 5;
                    int chunkZ = z - 5;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderGrassMesh(Chunk::GRASS_BOTTOM);
                    }
                }
            }
        }
        // Handle oak log blocks (different textures for top vs sides)
        else if (blockType == BlockType::OAK_LOG) {
            // Render oak log top/bottom faces
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // No tint
            glBindTexture(GL_TEXTURE_2D, m_oakLogTopTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_TOP);  // Render log top faces
                    }
                }
            }
            
            // Render oak log side faces
            glBindTexture(GL_TEXTURE_2D, m_oakLogSideTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_SIDE);  // Render log side faces
                    }
                }
            }
        }
        // Handle birch log blocks (different textures for top vs sides)
        else if (blockType == BlockType::BIRCH_LOG) {
            // Render birch log top/bottom faces
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // No tint
            glBindTexture(GL_TEXTURE_2D, m_birchLogTopTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_TOP);  // Render log top faces
                    }
                }
            }
            
            // Render birch log side faces
            glBindTexture(GL_TEXTURE_2D, m_birchLogSideTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_SIDE);  // Render log side faces
                    }
                }
            }
        }
        // Handle dark oak log blocks (different textures for top vs sides)
        else if (blockType == BlockType::DARK_OAK_LOG) {
            // Render dark oak log top/bottom faces
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // No tint
            glBindTexture(GL_TEXTURE_2D, m_darkOakLogTopTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_TOP);  // Render log top faces
                    }
                }
            }
            
            // Render dark oak log side faces
            glBindTexture(GL_TEXTURE_2D, m_darkOakLogSideTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 4;
                    int chunkZ = z - 4;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderLogMesh(Chunk::GRASS_SIDE);  // Render log side faces
                    }
                }
            }
        } else {
            // Get texture info for this block type
            const BlockTextureInfo& textureInfo = m_blockManager.GetTextureInfo(blockType);
            
            auto textureIt = m_blockTextures.find(blockType);
            if (textureIt != m_blockTextures.end() && textureIt->second != 0) {
                glBindTexture(GL_TEXTURE_2D, textureIt->second);
                
                // Check if this block type needs biome-based tinting (leaf blocks)
                if (NeedsBiomeTinting(blockType)) {
                    // Render chunks with biome-based tinting per chunk
                    for (int x = 0; x < WORLD_SIZE; ++x) {
                        for (int z = 0; z < WORLD_SIZE; ++z) {
                            int chunkX = x - 5;
                            int chunkZ = z - 5;
                            const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                            if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                                ApplyBiomeTinting(blockType, chunkX, chunkZ, world.GetSeed());
                                chunk->RenderMeshForBlockType(blockType);
                            }
                        }
                    }
                } else {
                    // Use the actual tint values from the block definition (for non-biome blocks)
                    glUniform3f(m_colorTintLoc, textureInfo.tintR, textureInfo.tintG, textureInfo.tintB);
                    
                    // Render all chunks for this block type with the same tint
                    for (int x = 0; x < WORLD_SIZE; ++x) {
                        for (int z = 0; z < WORLD_SIZE; ++z) {
                            // Convert array indices to chunk coordinates
                            // Array indices 0-9 map to chunk coordinates -5 to +4
                            int chunkX = x - 5;
                            int chunkZ = z - 5;
                            const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                            if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                                chunk->RenderMeshForBlockType(blockType);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Render water blocks with water shader (after opaque blocks for proper transparency)
    glUseProgram(m_waterShaderProgram);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing for water (but keep depth testing enabled)
    // This allows us to see blocks behind/underneath the water
    glDepthMask(GL_FALSE);
    
    // Set matrices for water shader
    glUniformMatrix4fv(m_waterViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_waterProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Set time uniform for water animation
    static float waterTime = 0.0f;
    waterTime += 0.016f; // ~60 FPS increment
    glUniform1f(m_waterTimeLoc, waterTime);
    
    // Set game time for sky color synchronization
    glUniform1f(m_waterGameTimeLoc, gameTime);
    
    // Set camera position for reflection calculations
    glUniform3f(m_waterCameraPosLoc, m_cameraPos.x, m_cameraPos.y, m_cameraPos.z);
    
    // Calculate sun direction based on game time (same as sky shader)
    float cycleTime = fmod(gameTime, 900.0f); // Get position within 15-minute cycle
    float timeAngle = (cycleTime / 900.0f) * 2.0f * M_PI; // Full circle over day cycle
    Vec3 sunDirection(
        sin(timeAngle),
        cos(timeAngle),
        0.0f
    );
    glUniform3f(m_waterSunDirLoc, sunDirection.x, sunDirection.y, sunDirection.z);
    
    // Set identity model matrix for water
    Mat4 waterModelMatrix;  // Identity matrix
    glUniformMatrix4fv(m_waterModelLoc, 1, GL_FALSE, waterModelMatrix.m);
    
    // Don't bind any texture for water - we want pure color
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Render water blocks
    std::vector<BlockType> waterBlocks = {BlockType::WATER_STILL, BlockType::WATER_FLOW};
    for (BlockType waterType : waterBlocks) {
        for (int x = 0; x < WORLD_SIZE; ++x) {
            for (int z = 0; z < WORLD_SIZE; ++z) {
                int chunkX = x - 5;
                int chunkZ = z - 5;
                const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                    chunk->RenderMeshForBlockType(waterType);
                }
            }
        }
    }
    
    // Re-enable depth writing and disable blending, switch back to regular shader
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(m_shaderProgram);
    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::RenderCube(float x, float y, float z) {
    // Legacy individual cube rendering (kept for compatibility)
    // Create translation matrix for this cube
    Mat4 modelMatrix = CreateTranslationMatrix(x, y, z);
    
    // Set model matrix uniform
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
    
    // Render cube
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    // Check for OpenGL errors (only for debugging)
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error in RenderCube: " << error << std::endl;
    }
}

void Renderer::RenderBlockWireframe(const Vec3& blockPos, const World& world) {
    // Check if the block position has a solid block
    Block block = world.GetBlock((int)blockPos.x, (int)blockPos.y, (int)blockPos.z);
    if (!block.IsSolid()) {
        return; // Don't render wireframe for air blocks
    }
    
    // Use wireframe shader program
    glUseProgram(m_wireframeShaderProgram);
    
    // Set view and projection matrices (same as current frame)
    glUniformMatrix4fv(m_wireframeViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_wireframeProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Create translation matrix for the block position
    Mat4 modelMatrix = CreateTranslationMatrix(blockPos.x, blockPos.y, blockPos.z);
    glUniformMatrix4fv(m_wireframeModelLoc, 1, GL_FALSE, modelMatrix.m);
    
    // Set up wireframe rendering
    glDisable(GL_CULL_FACE); // Disable face culling for wireframe
    glLineWidth(2.0f); // Make wireframe lines thicker
    
    // Render the complete cube outline (12 lines)
    glBindVertexArray(m_wireframeVAO);
    glDrawArrays(GL_LINES, 0, 24); // 12 lines * 2 vertices per line = 24 vertices
    glBindVertexArray(0);
    
    // Restore rendering state
    glLineWidth(1.0f); // Reset line width
    glEnable(GL_CULL_FACE); // Re-enable face culling
}

void Renderer::EndFrame() {
    // Ensure OpenGL state is properly reset after rendering
    glBindVertexArray(0);
    glUseProgram(0);
    
    // Restore viewport in case ImGui changed it
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

void Renderer::RenderTriangle() {
    // Legacy triangle rendering
    glUseProgram(m_shaderProgram);
    
    // Use identity matrices for 2D triangle
    Mat4 identity;
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, identity.m);
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, identity.m);
    glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, identity.m);
    
    glBindVertexArray(m_triangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

std::string Renderer::LoadShaderSource(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string source = buffer.str();
    DEBUG_SHADER("Loaded shader from: " << filepath);
    return source;
}

bool Renderer::CreateShaders() {
    // Load vertex shader source from file
    std::string vertexShaderSource = LoadShaderSource("shaders/vertex.glsl");
    if (vertexShaderSource.empty()) {
        std::cerr << "Failed to load vertex shader" << std::endl;
        return false;
    }

    // Load fragment shader source from file  
    std::string fragmentShaderSource = LoadShaderSource("shaders/fragment.glsl");
    if (fragmentShaderSource.empty()) {
        std::cerr << "Failed to load fragment shader" << std::endl;
        return false;
    }

    // Compile shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    // Check for linking errors
    if (!CheckProgramLinking(m_shaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DEBUG_SHADER("Shaders loaded and compiled successfully!");
    return true;
}

bool Renderer::CreatePlayerShaders() {
    // Load player vertex shader source from file
    std::string vertexShaderSource = LoadShaderSource("shaders/player_vertex.glsl");
    if (vertexShaderSource.empty()) {
        std::cerr << "Failed to load player vertex shader" << std::endl;
        return false;
    }

    // Load player fragment shader source from file  
    std::string fragmentShaderSource = LoadShaderSource("shaders/player_fragment.glsl");
    if (fragmentShaderSource.empty()) {
        std::cerr << "Failed to load player fragment shader" << std::endl;
        return false;
    }

    // Compile player shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create player shader program
    m_playerShaderProgram = glCreateProgram();
    glAttachShader(m_playerShaderProgram, vertexShader);
    glAttachShader(m_playerShaderProgram, fragmentShader);
    glLinkProgram(m_playerShaderProgram);

    // Check for linking errors
    if (!CheckProgramLinking(m_playerShaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_playerShaderProgram);
        m_playerShaderProgram = 0;
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DEBUG_SHADER("Player shaders loaded and compiled successfully!");
    return true;
}

bool Renderer::CreateWireframeShaders() {
    // Load wireframe vertex shader source from file
    std::string vertexShaderSource = LoadShaderSource("shaders/wireframe_vertex.glsl");
    if (vertexShaderSource.empty()) {
        std::cerr << "Failed to load wireframe vertex shader" << std::endl;
        return false;
    }

    // Load wireframe fragment shader source from file  
    std::string fragmentShaderSource = LoadShaderSource("shaders/wireframe_fragment.glsl");
    if (fragmentShaderSource.empty()) {
        std::cerr << "Failed to load wireframe fragment shader" << std::endl;
        return false;
    }

    // Compile wireframe shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create wireframe shader program
    m_wireframeShaderProgram = glCreateProgram();
    glAttachShader(m_wireframeShaderProgram, vertexShader);
    glAttachShader(m_wireframeShaderProgram, fragmentShader);
    glLinkProgram(m_wireframeShaderProgram);

    // Check for linking errors
    if (!CheckProgramLinking(m_wireframeShaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_wireframeShaderProgram);
        m_wireframeShaderProgram = 0;
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DEBUG_SHADER("Wireframe shaders loaded and compiled successfully!");
    return true;
}

bool Renderer::CreateSkyShaders() {
    // Load sky vertex shader source from file
    std::string vertexShaderSource = LoadShaderSource("shaders/sky_vertex.glsl");
    if (vertexShaderSource.empty()) {
        std::cerr << "Failed to load sky vertex shader" << std::endl;
        return false;
    }

    // Load sky fragment shader source from file  
    std::string fragmentShaderSource = LoadShaderSource("shaders/sky_fragment.glsl");
    if (fragmentShaderSource.empty()) {
        std::cerr << "Failed to load sky fragment shader" << std::endl;
        return false;
    }

    // Compile sky shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create sky shader program
    m_skyShaderProgram = glCreateProgram();
    glAttachShader(m_skyShaderProgram, vertexShader);
    glAttachShader(m_skyShaderProgram, fragmentShader);
    glLinkProgram(m_skyShaderProgram);

    // Check for linking errors
    if (!CheckProgramLinking(m_skyShaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_skyShaderProgram);
        m_skyShaderProgram = 0;
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DEBUG_SHADER("Sky shaders loaded and compiled successfully!");
    return true;
}

bool Renderer::CreateWaterShaders() {
    // Load water vertex shader source from file
    std::string vertexShaderSource = LoadShaderSource("shaders/water_vertex.glsl");
    if (vertexShaderSource.empty()) {
        std::cerr << "Failed to load water vertex shader" << std::endl;
        return false;
    }

    // Load water fragment shader source from file  
    std::string fragmentShaderSource = LoadShaderSource("shaders/water_fragment.glsl");
    if (fragmentShaderSource.empty()) {
        std::cerr << "Failed to load water fragment shader" << std::endl;
        return false;
    }

    // Compile water shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Create water shader program
    m_waterShaderProgram = glCreateProgram();
    glAttachShader(m_waterShaderProgram, vertexShader);
    glAttachShader(m_waterShaderProgram, fragmentShader);
    glLinkProgram(m_waterShaderProgram);

    // Check for linking errors
    if (!CheckProgramLinking(m_waterShaderProgram)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_waterShaderProgram);
        m_waterShaderProgram = 0;
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DEBUG_SHADER("Water shaders loaded and compiled successfully!");
    return true;
}

bool Renderer::CreateWireframeGeometry() {
    // Wireframe cube edges (12 lines for cube outline)
    float wireframeVertices[] = {
        // Bottom face edges (4 lines)
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, // Bottom front edge
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, // Bottom right edge
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, // Bottom back edge
        -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, // Bottom left edge
        
        // Top face edges (4 lines)
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, // Top front edge
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f, // Top right edge
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, // Top back edge
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, // Top left edge
        
        // Vertical edges (4 lines)
        -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, // Front left vertical
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, // Front right vertical
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, // Back right vertical
        -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f  // Back left vertical
    };

    glGenVertexArrays(1, &m_wireframeVAO);
    glBindVertexArray(m_wireframeVAO);

    glGenBuffers(1, &m_wireframeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_wireframeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wireframeVertices), wireframeVertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool Renderer::CreateSkyGeometry() {
    // Create a proper skybox cube with correct winding order for inside viewing
    // Using unit cube coordinates - shader will handle making it infinite
    float skyVertices[] = {        
        // Right face (+X) - viewed from inside
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        // Left face (-X) - viewed from inside
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // Top face (+Y) - viewed from inside
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        // Bottom face (-Y) - viewed from inside
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,

        // Back face (+Z) - viewed from inside
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // Front face (-Z) - viewed from inside
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f
    };

    glGenVertexArrays(1, &m_skyVAO);
    glBindVertexArray(m_skyVAO);

    glGenBuffers(1, &m_skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVertices), skyVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    DEBUG_SHADER("Sky geometry created successfully!");
    return true;
}

unsigned int Renderer::CompileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    if (!CheckShaderCompilation(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Renderer::CheckShaderCompilation(unsigned int shader, const char* type) {
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
        return false;
    }
    return true;
}

bool Renderer::CheckProgramLinking(unsigned int program) {
    int success;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
        return false;
    }
    return true;
}

Mat4 Renderer::CreateProjectionMatrix(float fov, float aspect, float near, float far) {
    Mat4 proj;
    
    float tanFov = tan(fov * M_PI / 360.0f); // fov/2 in radians (treating as vertical FOV)
    
    proj.m[0] = 1.0f / (aspect * tanFov);
    proj.m[5] = 1.0f / tanFov;
    proj.m[10] = -(far + near) / (far - near);
    proj.m[11] = -1.0f;
    proj.m[14] = -(2.0f * far * near) / (far - near);
    proj.m[15] = 0.0f;
    
    return proj;
}

Mat4 Renderer::CreateTranslationMatrix(float x, float y, float z) {
    Mat4 trans;
    trans.m[12] = x;
    trans.m[13] = y;
    trans.m[14] = z;
    return trans;
}

void Renderer::RenderOtherPlayers(const std::vector<PlayerPosition>& playerPositions) {
    if (playerPositions.empty()) {
        return; // No other players to render
    }
    

    
    // Set the view and projection matrices for player rendering
    glUseProgram(m_playerShaderProgram);
    glUniformMatrix4fv(m_playerViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_playerProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Render each other player with the Minecraft-like player model
    for (const PlayerPosition& playerPos : playerPositions) {
        

        
        // Render player model at their position with their rotation
        Vec3 position(playerPos.x, playerPos.y, playerPos.z);
        
        m_playerModel.Render(position, playerPos.yaw, playerPos.pitch);
    }
    

}

void Renderer::RenderFirstPersonArm(const Player& player) {
    // Set up the player shader program for first-person arm rendering
    glUseProgram(m_playerShaderProgram);
    
    // For first-person arm, use identity view matrix so it's not affected by camera rotation
    // This makes it stay fixed relative to the screen like in Minecraft
    Mat4 identityView;
    // Identity matrix is already initialized in Mat4 constructor
    
    glUniformMatrix4fv(m_playerViewLoc, 1, GL_FALSE, identityView.m);
    glUniformMatrix4fv(m_playerProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Disable depth testing completely for the arm so it always shows up
    glDisable(GL_DEPTH_TEST);
    
    // Debug output to verify arm is being rendered
    static int renderCount = 0;
    if (renderCount < 5) {
        std::cout << "[DEBUG] Rendering first-person arm (frame " << renderCount << ")" << std::endl;
        renderCount++;
    }
    
    // Render the first-person arm using the player model
    m_playerModel.RenderFirstPersonArm(player);
    
    // Re-enable depth testing for subsequent rendering
    glEnable(GL_DEPTH_TEST);
    
    // Switch back to main shader program for subsequent rendering
    glUseProgram(m_shaderProgram);
}

unsigned int Renderer::LoadTexture(const std::string& filepath) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Load image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) {
            // For grayscale images, force load as RGB to replicate grayscale value across R, G, B channels
            stbi_image_free(data); // Free the original single-channel data
            data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 3); // Force RGB
            format = GL_RGB;
        } else if (nrChannels == 2) {
            format = GL_RG;
        } else if (nrChannels == 4) {
            format = GL_RGBA;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        DEBUG_TEXTURE("Loaded texture: " << filepath << " (" << width << "x" << height << ", " << nrChannels << " channels)");
    } else {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

unsigned int Renderer::LoadTextureWithAlpha(const std::string& filepath) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Load image and force it to RGBA
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 4); // Force 4 channels
    
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        DEBUG_TEXTURE("Loaded texture with alpha: " << filepath << " (" << width << "x" << height << ", forced to 4 channels)");
    } else {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

bool Renderer::LoadBlockTextures() {
    // Load block definitions from JSON
    if (!m_blockManager.LoadBlockDefinitions("blocks_config.json")) {
        std::cerr << "Failed to load block definitions from JSON" << std::endl;
        return false;
    }
    
    // Load textures for all block types
    std::vector<BlockType> allBlockTypes = m_blockManager.GetAllBlockTypes();
    
    for (BlockType blockType : allBlockTypes) {
        const BlockTextureInfo& textureInfo = m_blockManager.GetTextureInfo(blockType);
        
        // Skip AIR blocks
        if (blockType == BlockType::AIR) {
            m_blockTextures[blockType] = 0;
            continue;
        }
        
        // Handle blocks with special texture requirements (like grass)
        if (blockType == BlockType::GRASS) {
            // Load grass textures (already handled by default blocks)
            m_grassTopTexture = LoadTexture("assets/block/" + textureInfo.top);
            m_grassSideTexture = LoadTexture("assets/block/" + textureInfo.sides);
            m_grassSideOverlayTexture = LoadTextureWithAlpha("assets/block/" + textureInfo.overlay);
            m_grassBottomTexture = LoadTexture("assets/block/" + textureInfo.bottom);
            m_blockTextures[blockType] = m_grassSideTexture;
        }
        // Handle log blocks (different textures for top/bottom vs sides)
        else if (blockType == BlockType::OAK_LOG) {
            m_oakLogTopTexture = LoadTexture("assets/block/" + textureInfo.top);
            m_oakLogSideTexture = LoadTexture("assets/block/" + textureInfo.sides);
            m_blockTextures[blockType] = m_oakLogSideTexture;
        }
        else if (blockType == BlockType::BIRCH_LOG) {
            m_birchLogTopTexture = LoadTexture("assets/block/" + textureInfo.top);
            m_birchLogSideTexture = LoadTexture("assets/block/" + textureInfo.sides);
            m_blockTextures[blockType] = m_birchLogSideTexture;
        }
        else if (blockType == BlockType::DARK_OAK_LOG) {
            m_darkOakLogTopTexture = LoadTexture("assets/block/" + textureInfo.top);
            m_darkOakLogSideTexture = LoadTexture("assets/block/" + textureInfo.sides);
            m_blockTextures[blockType] = m_darkOakLogSideTexture;
        }
        // Handle blocks with a single texture for all faces
        else if (!textureInfo.all.empty()) {
            unsigned int textureID = LoadTexture("assets/block/" + textureInfo.all);
            m_blockTextures[blockType] = textureID;
            
            if (textureID == 0) {
                std::cerr << "Failed to load texture for block type " << static_cast<int>(blockType) 
                          << " (" << m_blockManager.GetBlockNameByType(blockType) << ")" << std::endl;
            }
        }
        // Handle blocks with separate textures for different faces (future enhancement)
        else if (!textureInfo.sides.empty()) {
            // For now, just use the sides texture for all faces
            unsigned int textureID = LoadTexture("assets/block/" + textureInfo.sides);
            m_blockTextures[blockType] = textureID;
        }
        else {
            // No texture specified, use a default or skip
            std::cout << "Warning: No texture specified for block " 
                      << m_blockManager.GetBlockNameByType(blockType) << std::endl;
            m_blockTextures[blockType] = 0;
        }
    }
    
    DEBUG_TEXTURE("Loaded textures for " << m_blockTextures.size() << " block types");
    return true;
}

bool Renderer::LoadSkyTextures() {
    // Load sun texture
    m_sunTexture = LoadTexture("assets/environment/sun.png");
    if (m_sunTexture == 0) {
        std::cerr << "Failed to load sun texture" << std::endl;
        return false;
    }
    
    // Load moon texture (use top-left moon from the 4x2 grid)
    m_moonTexture = LoadTexture("assets/environment/moon_phases.png");
    if (m_moonTexture == 0) {
        std::cerr << "Failed to load moon texture" << std::endl;
        return false;
    }
    
    DEBUG_TEXTURE("Sky textures loaded successfully!");
    return true;
}

bool Renderer::LoadHotbarTextures() {
    // Load hotbar texture with alpha support
    m_hotbarTexture = LoadTextureWithAlpha("assets/gui/sprites/hud/hotbar.png");
    if (m_hotbarTexture == 0) {
        std::cerr << "Failed to load hotbar texture" << std::endl;
        return false;
    }
    
    // Load hotbar selection texture
    m_hotbarSelectionTexture = LoadTextureWithAlpha("assets/gui/sprites/hud/hotbar_selection.png");
    if (m_hotbarSelectionTexture == 0) {
        std::cerr << "Failed to load hotbar selection texture" << std::endl;
        return false;  
    }
    
    DEBUG_TEXTURE("Hotbar textures loaded successfully!");
    return true;
}

bool Renderer::LoadInventoryTextures() {
    // Load inventory texture with alpha support
    m_inventoryTexture = LoadTextureWithAlpha("assets/gui/container/inventory.png");
    if (m_inventoryTexture == 0) {
        std::cerr << "Failed to load inventory texture" << std::endl;
        return false;
    }
    
    DEBUG_TEXTURE("Inventory textures loaded successfully!");
    return true;
}

void Renderer::RenderSky(float gameTime) {
    // Debug output only when time changes significantly
    static float lastDebugTime = -1.0f;
    if (abs(gameTime - lastDebugTime) > 10.0f) {
        std::cout << "[RENDERER] Sky time: " << gameTime << " seconds" << std::endl;
        lastDebugTime = gameTime;
    }
    
    // Set a neutral clear color as fallback (will be overridden by sky rendering)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Save current rendering state
    bool depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    bool cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    
    // Disable depth testing, depth writing, and face culling for sky rendering
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE); // Disable culling so we can see inside faces of skybox
    
    // Use sky shader program
    glUseProgram(m_skyShaderProgram);
    
    // Set uniforms
    glUniformMatrix4fv(m_skyViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_skyProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    glUniform1f(m_skyGameTimeLoc, gameTime);
    
    // Calculate sun direction based on time (for potential future use)
    float cycleTime = fmod(gameTime, 900.0f); // Get position within 15-minute cycle
    float timeAngle = (cycleTime / 900.0f) * 2.0f * M_PI; // Full circle over day cycle
    Vec3 sunDirection(
        sin(timeAngle),
        cos(timeAngle),
        0.0f
    );
    glUniform3f(m_skySunDirLoc, sunDirection.x, sunDirection.y, sunDirection.z);
    
    // Render sky geometry
    glBindVertexArray(m_skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices for cube (6 faces * 6 vertices per face)
    glBindVertexArray(0);
    
    // Restore rendering state
    glDepthMask(GL_TRUE);
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    if (cullFaceEnabled) {
        glEnable(GL_CULL_FACE);
    }
    
    // Switch back to main shader program for subsequent rendering
    glUseProgram(m_shaderProgram);
}

// ============== FRUSTUM CULLING IMPLEMENTATION ==============

void Renderer::ExtractFrustum(const Mat4& viewMatrix, const Mat4& projMatrix) {
    // Multiply view and projection matrices to get view-projection matrix
    Mat4 viewProj;
    
    // Manual matrix multiplication: viewProj = proj * view
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            viewProj.m[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                viewProj.m[i * 4 + j] += projMatrix.m[i * 4 + k] * viewMatrix.m[k * 4 + j];
            }
        }
    }
    
    // Extract the six frustum planes from the view-projection matrix
    // Left plane
    m_frustum.planes[0] = Plane(
        Vec3(viewProj.m[3] + viewProj.m[0], viewProj.m[7] + viewProj.m[4], viewProj.m[11] + viewProj.m[8]),
        viewProj.m[15] + viewProj.m[12]
    );
    
    // Right plane
    m_frustum.planes[1] = Plane(
        Vec3(viewProj.m[3] - viewProj.m[0], viewProj.m[7] - viewProj.m[4], viewProj.m[11] - viewProj.m[8]),
        viewProj.m[15] - viewProj.m[12]
    );
    
    // Bottom plane
    m_frustum.planes[2] = Plane(
        Vec3(viewProj.m[3] + viewProj.m[1], viewProj.m[7] + viewProj.m[5], viewProj.m[11] + viewProj.m[9]),
        viewProj.m[15] + viewProj.m[13]
    );
    
    // Top plane
    m_frustum.planes[3] = Plane(
        Vec3(viewProj.m[3] - viewProj.m[1], viewProj.m[7] - viewProj.m[5], viewProj.m[11] - viewProj.m[9]),
        viewProj.m[15] - viewProj.m[13]
    );
    
    // Near plane
    m_frustum.planes[4] = Plane(
        Vec3(viewProj.m[3] + viewProj.m[2], viewProj.m[7] + viewProj.m[6], viewProj.m[11] + viewProj.m[10]),
        viewProj.m[15] + viewProj.m[14]
    );
    
    // Far plane
    m_frustum.planes[5] = Plane(
        Vec3(viewProj.m[3] - viewProj.m[2], viewProj.m[7] - viewProj.m[6], viewProj.m[11] - viewProj.m[10]),
        viewProj.m[15] - viewProj.m[14]
    );
    
    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        float length = m_frustum.planes[i].normal.Length();
        if (length > 0.0f) {
            m_frustum.planes[i].normal = m_frustum.planes[i].normal * (1.0f / length);
            m_frustum.planes[i].distance /= length;
        }
    }
}

bool Renderer::IsChunkInFrustum(int chunkX, int chunkZ) const {
    // Calculate chunk world bounds
    // Each chunk is 16x16 blocks in XZ, full height (256) in Y
    float worldX = chunkX * CHUNK_WIDTH;
    float worldZ = chunkZ * CHUNK_DEPTH;
    
    Vec3 chunkMin(worldX, 0.0f, worldZ);
    Vec3 chunkMax(worldX + CHUNK_WIDTH, CHUNK_HEIGHT, worldZ + CHUNK_DEPTH); 
    
    AABB chunkAABB(chunkMin, chunkMax);
    
    // Make culling much more conservative by expanding the AABB significantly
    // This prevents false positives where chunks at the edge get culled incorrectly
            // Using a large margin because 70° FOV is still quite wide
    float margin = 16.0f; // 16 block margin - very conservative
    AABB expandedAABB(Vec3(chunkMin.x - margin, chunkMin.y - margin, chunkMin.z - margin),
                      Vec3(chunkMax.x + margin, chunkMax.y + margin, chunkMax.z + margin));
    
    bool inFrustum = IsAABBInFrustum(expandedAABB);
    
    // Debug output for the first few seconds to see culling behavior
    static float debugStartTime = glfwGetTime();
    if (glfwGetTime() - debugStartTime < 5.0f) { // Debug for first 5 seconds
        if (!inFrustum) {
            static int debugCount = 0;
            if (debugCount < 10) { // Limit debug spam
                std::cout << "[FRUSTUM] Culling chunk (" << chunkX << ", " << chunkZ 
                          << ") at world coords (" << worldX << ", " << worldZ << ")" << std::endl;
                debugCount++;
            }
        }
    }
    
    return inFrustum;
}

bool Renderer::IsAABBInFrustum(const AABB& aabb) const {
    // Test the AABB against all 6 frustum planes
    // If the box is completely behind any plane, it's outside the frustum
    
    for (int i = 0; i < 6; i++) {
        const Plane& plane = m_frustum.planes[i];
        
        // Find the positive vertex (furthest point in direction of plane normal)
        Vec3 positiveVertex;
        positiveVertex.x = (plane.normal.x >= 0.0f) ? aabb.max.x : aabb.min.x;
        positiveVertex.y = (plane.normal.y >= 0.0f) ? aabb.max.y : aabb.min.y;
        positiveVertex.z = (plane.normal.z >= 0.0f) ? aabb.max.z : aabb.min.z;
        
        // If positive vertex is behind the plane, the entire AABB is outside
        if (plane.DistanceToPoint(positiveVertex) < 0.0f) {
            return false;
        }
    }
    
    return true; // AABB is inside or intersecting the frustum
}

bool Renderer::NeedsBiomeTinting(BlockType blockType) const {
    return blockType == BlockType::GRASS || IsLeafBlock(blockType);
}

bool Renderer::IsLeafBlock(BlockType blockType) const {
    return blockType == BlockType::ACACIA_LEAVES ||
           blockType == BlockType::AZALEA_LEAVES ||
           blockType == BlockType::BIRCH_LEAVES ||
           blockType == BlockType::CHERRY_LEAVES ||
           blockType == BlockType::JUNGLE_LEAVES ||
           blockType == BlockType::MANGROVE_LEAVES ||
           blockType == BlockType::SPRUCE_LEAVES ||
           blockType == static_cast<BlockType>(235); // OAK_LEAVES
}

void Renderer::ApplyBiomeTinting(BlockType blockType, int chunkX, int chunkZ, int worldSeed) {
    // Get center position of chunk for biome calculation
    int worldX = chunkX * 16 + 8; // CHUNK_WIDTH is 16
    int worldZ = chunkZ * 16 + 8; // CHUNK_DEPTH is 16
    
    // Get biome for this chunk
    BiomeType biome = BiomeSystem::GetBiomeType(worldX, worldZ, worldSeed);
    
    float r, g, b;
    
    if (blockType == BlockType::GRASS) {
        // Apply grass color for this biome
        BiomeSystem::GetGrassColor(biome, r, g, b);
    } else if (IsLeafBlock(blockType)) {
        // Apply foliage color for this biome
        BiomeSystem::GetFoliageColor(biome, r, g, b);
    } else {
        // Fallback to no tint
        r = g = b = 1.0f;
    }
    
    // Apply the tint to the shader
    glUniform3f(m_colorTintLoc, r, g, b);
}

// Item texture management methods
unsigned int Renderer::LoadItemTexture(const std::string& itemIconPath) {
    // Check if texture is already loaded
    auto it = m_itemTextures.find(itemIconPath);
    if (it != m_itemTextures.end()) {
        return it->second;
    }
    
    // Load the texture
    std::string fullPath = "assets/" + itemIconPath;
    unsigned int textureId = LoadTextureWithAlpha(fullPath);
    
    // Cache it for future use
    if (textureId != 0) {
        m_itemTextures[itemIconPath] = textureId;
        std::cout << "Loaded item texture: " << fullPath << std::endl;
    } else {
        std::cerr << "Failed to load item texture: " << fullPath << std::endl;
    }
    
    return textureId;
}

unsigned int Renderer::GetItemTexture(const std::string& itemIconPath) {
    // Check if texture is already loaded
    auto it = m_itemTextures.find(itemIconPath);
    if (it != m_itemTextures.end()) {
        return it->second;
    }
    
    // If not loaded, load it now
    return LoadItemTexture(itemIconPath);
}

void Renderer::UpdateFirstPersonArm(float deltaTime) {
    m_playerModel.UpdateAnimation(deltaTime);
}

void Renderer::TriggerArmPunch() {
    m_playerModel.TriggerPunchAnimation();
}
