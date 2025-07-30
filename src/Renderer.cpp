#include "Renderer.h"
#include "World.h"
#include "Chunk.h"
#include "PlayerModel.h"
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

Renderer::Renderer() : m_cubeVAO(0), m_cubeVBO(0), m_shaderProgram(0), 
                       m_triangleVAO(0), m_triangleVBO(0),
                       m_wireframeVAO(0), m_wireframeVBO(0), m_wireframeShaderProgram(0),
                       m_wireframeModelLoc(-1), m_wireframeViewLoc(-1), m_wireframeProjLoc(-1),
                       m_playerShaderProgram(0),
                       m_playerModelLoc(-1), m_playerViewLoc(-1), m_playerProjLoc(-1),
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

    // Get uniform locations for block shaders
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    
    // Get uniform locations for player shaders
    m_playerModelLoc = glGetUniformLocation(m_playerShaderProgram, "model");
    m_playerViewLoc = glGetUniformLocation(m_playerShaderProgram, "view");
    m_playerProjLoc = glGetUniformLocation(m_playerShaderProgram, "projection");
    
    // Get uniform locations for wireframe shaders
    m_wireframeModelLoc = glGetUniformLocation(m_wireframeShaderProgram, "model");
    m_wireframeViewLoc = glGetUniformLocation(m_wireframeShaderProgram, "view");
    m_wireframeProjLoc = glGetUniformLocation(m_wireframeShaderProgram, "projection");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
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

    std::cout << "3D Renderer initialized successfully!" << std::endl;
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
    // Dynamic sky color will be handled in RenderSky()
    // For now, use a neutral color that will be overridden
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Sky blue background (#87CEEB)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Ensure depth testing is enabled
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void Renderer::SetViewport(int width, int height) {
    std::cout << "Setting viewport to: " << width << "x" << height << std::endl;
    
    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
    
    // Update projection matrix
    float aspect = (float)width / (float)height;
    m_projectionMatrix = CreateProjectionMatrix(65.0f, aspect, 0.1f, 100.0f);
    
    std::cout << "Aspect ratio: " << aspect << std::endl;
}

void Renderer::BeginFrame(const Player& player) {
    // Ensure viewport is correct (in case ImGui or other code changed it)
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    glUseProgram(m_shaderProgram);
    
    // Set view matrix and store it for player rendering
    m_viewMatrix = player.GetViewMatrix();
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, m_viewMatrix.m);
    
    // Set projection matrix
    glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Extract frustum planes for culling
    ExtractFrustum(m_viewMatrix, m_projectionMatrix);
}

void Renderer::RenderWorld(const World& world) {
    // Use optimized chunk-based rendering instead of individual cubes
    RenderChunks(world);
}

void Renderer::RenderChunks(const World& world) {
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
        // Skip AIR blocks
        if (blockType == BlockType::AIR) {
            continue;
        }
        
        // Handle grass blocks specially (different textures per face)
        if (blockType == BlockType::GRASS) {
            // Render grass top faces with purple tint
            glUniform3f(m_colorTintLoc, 0.3f, 0.7f, 0.4f); // green
            glBindTexture(GL_TEXTURE_2D, m_grassTopTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 3;
                    int chunkZ = z - 3;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderGrassMesh(Chunk::GRASS_TOP);
                    }
                }
            }
            
            // Render grass side faces with base texture (no tint)
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
            glBindTexture(GL_TEXTURE_2D, m_grassSideTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 3;
                    int chunkZ = z - 3;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderGrassMesh(Chunk::GRASS_SIDE);
                    }
                }
            }
            
            // Render grass side overlay on top using polygon offset to avoid z-fighting
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f); // Pull overlay slightly toward camera
            glUniform3f(m_colorTintLoc, 0.3f, 0.7f, 0.4f); // grass mult
            glBindTexture(GL_TEXTURE_2D, m_grassSideOverlayTexture);
            for (int x = 0; x < WORLD_SIZE; ++x) {
                for (int z = 0; z < WORLD_SIZE; ++z) {
                    int chunkX = x - 3;
                    int chunkZ = z - 3;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
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
                    int chunkX = x - 3;
                    int chunkZ = z - 3;
                    const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                    if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                        chunk->RenderGrassMesh(Chunk::GRASS_BOTTOM);
                    }
                }
            }
        } else {
            // Bind the appropriate texture for this block type (no tint)
            glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
            
            auto textureIt = m_blockTextures.find(blockType);
            if (textureIt != m_blockTextures.end() && textureIt->second != 0) {
                glBindTexture(GL_TEXTURE_2D, textureIt->second);
                
                // Render all chunks for this block type
                for (int x = 0; x < WORLD_SIZE; ++x) {
                    for (int z = 0; z < WORLD_SIZE; ++z) {
                        // Convert array indices to chunk coordinates
                        // Array indices 0-5 map to chunk coordinates -3 to +2
                        int chunkX = x - 3;
                        int chunkZ = z - 3;
                        const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
                        if (chunk && chunk->HasMesh() && (!m_enableFrustumCulling || IsChunkInFrustum(chunkX, chunkZ))) {
                            chunk->RenderMeshForBlockType(blockType);
                        }
                    }
                }
            }
        }
    }
    
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Render as wireframe
    glLineWidth(2.0f); // Make wireframe lines thicker
    
    // Render only visible faces by checking neighboring blocks
    glBindVertexArray(m_wireframeVAO);
    
    // Check each face and render only if it's visible (neighboring block is air)
    int faceOffset = 0;
    
    // Front face (+Z)
    Block frontNeighbor = world.GetBlock((int)blockPos.x, (int)blockPos.y, (int)blockPos.z + 1);
    if (!frontNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    faceOffset += 6;
    
    // Back face (-Z)
    Block backNeighbor = world.GetBlock((int)blockPos.x, (int)blockPos.y, (int)blockPos.z - 1);
    if (!backNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    faceOffset += 6;
    
    // Left face (-X)
    Block leftNeighbor = world.GetBlock((int)blockPos.x - 1, (int)blockPos.y, (int)blockPos.z);
    if (!leftNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    faceOffset += 6;
    
    // Right face (+X)
    Block rightNeighbor = world.GetBlock((int)blockPos.x + 1, (int)blockPos.y, (int)blockPos.z);
    if (!rightNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    faceOffset += 6;
    
    // Bottom face (-Y)
    Block bottomNeighbor = world.GetBlock((int)blockPos.x, (int)blockPos.y - 1, (int)blockPos.z);
    if (!bottomNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    faceOffset += 6;
    
    // Top face (+Y)
    Block topNeighbor = world.GetBlock((int)blockPos.x, (int)blockPos.y + 1, (int)blockPos.z);
    if (!topNeighbor.IsSolid()) {
        glDrawArrays(GL_TRIANGLES, faceOffset, 6);
    }
    
    glBindVertexArray(0);
    
    // Restore rendering state
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Back to filled polygons
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
    std::cout << "Loaded shader from: " << filepath << std::endl;
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

    std::cout << "Shaders loaded and compiled successfully!" << std::endl;
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

    std::cout << "Player shaders loaded and compiled successfully!" << std::endl;
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

    std::cout << "Wireframe shaders loaded and compiled successfully!" << std::endl;
    return true;
}

bool Renderer::CreateWireframeGeometry() {
    // Wireframe cube vertices (only positions)
    float wireframeVertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Back face
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Left face
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // Right face
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        // Top face
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
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

void Renderer::RenderOtherPlayers(const std::unordered_map<uint32_t, PlayerPosition>& otherPlayers) {
    if (otherPlayers.empty()) {
        return; // No other players to render
    }
    

    
    // Set the view and projection matrices for player rendering
    glUseProgram(m_playerShaderProgram);
    glUniformMatrix4fv(m_playerViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_playerProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Render each other player with the Minecraft-like player model
    for (const auto& pair : otherPlayers) {
        const PlayerPosition& playerPos = pair.second;
        

        
        // Render player model at their position with their rotation
        Vec3 position(playerPos.x, playerPos.y, playerPos.z);
        
        m_playerModel.Render(position, playerPos.yaw, playerPos.pitch);
    }
    

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
            format = GL_RED;
        } else if (nrChannels == 2) {
            format = GL_RG;
        } else if (nrChannels == 4) {
            format = GL_RGBA;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Loaded texture: " << filepath << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
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
        
        std::cout << "Loaded texture with alpha: " << filepath << " (" << width << "x" << height << ", forced to 4 channels)" << std::endl;
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
    
    std::cout << "Loaded textures for " << m_blockTextures.size() << " block types" << std::endl;
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
    
    std::cout << "Sky textures loaded successfully!" << std::endl;
    return true;
}

void Renderer::RenderSky(float gameTime) {
    // Debug output only when time changes significantly
    static float lastDebugTime = -1.0f;
    if (abs(gameTime - lastDebugTime) > 10.0f) {
        std::cout << "[RENDERER] Sky time: " << gameTime << " seconds" << std::endl;
        lastDebugTime = gameTime;
    }
    
    // Calculate time factor (0.0 = start of day, 1.0 = end of day cycle)
    float timeFactor = gameTime / 900.0f; // 900 seconds = 15 minutes
    
    // Determine sky color based on time
    float r, g, b;
    if (gameTime < 450.0f) {
        // Day time (0-450 seconds) - bright blue sky
        float dayProgress = gameTime / 450.0f;
        
        // Sunrise/sunset transitions
        if (dayProgress < 0.1f) {
            // Sunrise (orange to blue)
            float sunrise = dayProgress / 0.1f;
            r = 1.0f - (sunrise * 0.471f); // 1.0 to 0.529
            g = 0.5f + (sunrise * 0.308f); // 0.5 to 0.808  
            b = 0.2f + (sunrise * 0.722f); // 0.2 to 0.922
        } else if (dayProgress > 0.9f) {
            // Sunset (blue to orange)
            float sunset = (dayProgress - 0.9f) / 0.1f;
            r = 0.529f + (sunset * 0.471f); // 0.529 to 1.0
            g = 0.808f - (sunset * 0.308f); // 0.808 to 0.5
            b = 0.922f - (sunset * 0.722f); // 0.922 to 0.2
        } else {
            // Midday - bright blue
            r = 0.529f; g = 0.808f; b = 0.922f;
        }
    } else {
        // Night time (450-900 seconds) - dark blue/black sky
        float nightProgress = (gameTime - 450.0f) / 450.0f;
        
        if (nightProgress < 0.1f) {
            // Transition from sunset to night
            float transition = nightProgress / 0.1f;
            r = 1.0f - (transition * 0.9f); // 1.0 to 0.1
            g = 0.5f - (transition * 0.4f); // 0.5 to 0.1
            b = 0.2f - (transition * 0.1f); // 0.2 to 0.1
        } else if (nightProgress > 0.9f) {
            // Transition from night to sunrise
            float transition = (nightProgress - 0.9f) / 0.1f;
            r = 0.1f + (transition * 0.9f); // 0.1 to 1.0
            g = 0.1f + (transition * 0.4f); // 0.1 to 0.5
            b = 0.1f + (transition * 0.1f); // 0.1 to 0.2
        } else {
            // Midnight - dark blue
            r = 0.1f; g = 0.1f; b = 0.2f;
        }
    }
    
    // Set the clear color to our calculated sky color
    glClearColor(r, g, b, 1.0f);
    
    // Clear the color buffer with the new sky color
    glClear(GL_COLOR_BUFFER_BIT);
    
    // TODO: Add actual sun/moon rendering with textured quads
    // For now, we're just changing the sky color
    // Future implementation will render sun/moon sprites positioned in the sky
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