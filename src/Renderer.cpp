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
                       m_playerShaderProgram(0),
                       m_playerModelLoc(-1), m_playerViewLoc(-1), m_playerProjLoc(-1),
                       m_viewportWidth(1280), m_viewportHeight(720), m_renderMode(RenderMode::WHITE_ONLY), m_fadeFactor(0.0f) {
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

    // Get uniform locations for block shaders
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    
    // Get uniform locations for player shaders
    m_playerModelLoc = glGetUniformLocation(m_playerShaderProgram, "model");
    m_playerViewLoc = glGetUniformLocation(m_playerShaderProgram, "view");
    m_playerProjLoc = glGetUniformLocation(m_playerShaderProgram, "projection");

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
    
    // Get texture uniform location after loading textures
    m_textureLoc = glGetUniformLocation(m_shaderProgram, "blockTexture");
    m_colorTintLoc = glGetUniformLocation(m_shaderProgram, "colorTint");
    m_enableAOLoc = glGetUniformLocation(m_shaderProgram, "enableAO");
    m_enableTextureLoc = glGetUniformLocation(m_shaderProgram, "enableTexture");
    m_fadeFactorLoc = glGetUniformLocation(m_shaderProgram, "fadeFactor");
    
    // Initialize player model
    if (!m_playerModel.Initialize()) {
        std::cerr << "Failed to initialize player model" << std::endl;
        return false;
    }
    
    // Set up player model with shader program and uniform locations
    m_playerModel.UseShaderProgram(m_playerShaderProgram);
    m_playerModel.SetUniformLocations(m_playerModelLoc, m_playerViewLoc, m_playerProjLoc);
    
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
    
    // Clean up player model
    m_playerModel.Shutdown();
    
    // Clean up textures
    for (unsigned int texture : m_blockTextures) {
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
}

void Renderer::Clear() {
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
    m_projectionMatrix = CreateProjectionMatrix(75.0f, aspect, 0.1f, 100.0f);
    
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
}

void Renderer::RenderWorld(const World& world) {
    // Use optimized chunk-based rendering instead of individual cubes
    RenderChunks(world);
}

void Renderer::RenderChunks(const World& world) {
    // Set identity model matrix since chunks handle their own world positioning
    Mat4 modelMatrix;  // Identity matrix
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
    
    // Set fade factor uniform for smooth transitions
    glUniform1f(m_fadeFactorLoc, m_fadeFactor);
    
    // Configure rendering based on current mode
    if (m_renderMode == RenderMode::WHITE_ONLY) {
        // White only mode: disable textures and AO, render everything pure white
        glUniform1f(m_enableTextureLoc, 0.0f); // Disable textures
        glUniform1f(m_enableAOLoc, 0.0f);     // Disable ambient occlusion
        glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // Pure white
    } else if (m_renderMode == RenderMode::AO_ONLY) {
        // AO only mode: disable textures but enable AO
        glUniform1f(m_enableTextureLoc, 0.0f); // Disable textures
        glUniform1f(m_enableAOLoc, 1.0f);     // Enable ambient occlusion
        glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White base color
    } else {
        // Full render mode: enable both textures and AO
        glUniform1f(m_enableTextureLoc, 1.0f); // Enable textures
        glUniform1f(m_enableAOLoc, 1.0f);     // Enable ambient occlusion
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(m_textureLoc, 0);
    }
    
    // Define the block types we want to render in order
    std::vector<BlockType> blockTypesToRender = {BlockType::STONE, BlockType::DIRT, BlockType::GRASS};
    
    // Render each block type separately
    for (BlockType blockType : blockTypesToRender) {
        // Skip AIR blocks
        if (blockType == BlockType::AIR) {
            continue;
        }
        
        // Handle grass blocks specially (different textures per face)
        if (blockType == BlockType::GRASS) {
            // Render only the center chunk (0,0) for static camera mode
            int x = 3, z = 3;
            int chunkX = x - 3; // = 0
            int chunkZ = z - 3; // = 0
            const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
            
            if (chunk && chunk->HasMesh()) {
                if (m_renderMode == RenderMode::WHITE_ONLY) {
                    // White only: render all grass faces with white color, no textures
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f);
                    chunk->RenderGrassFace(Chunk::GRASS_TOP);
                    chunk->RenderGrassFace(Chunk::GRASS_SIDE);
                    chunk->RenderGrassFace(Chunk::GRASS_BOTTOM);
                } else if (m_renderMode == RenderMode::AO_ONLY) {
                    // AO only: render with AO but no textures
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f);
                    chunk->RenderGrassFace(Chunk::GRASS_TOP);
                    chunk->RenderGrassFace(Chunk::GRASS_SIDE);
                    chunk->RenderGrassFace(Chunk::GRASS_BOTTOM);
                } else {
                    // Full render: normal grass rendering with textures and AO
                    // Render grass top faces with purple tint
                    glUniform3f(m_colorTintLoc, 0.8f, 0.3f, 0.8f); // Purple tint
                    glBindTexture(GL_TEXTURE_2D, m_grassTopTexture);
                    chunk->RenderGrassFace(Chunk::GRASS_TOP);
                    
                    // Render grass side faces with base texture (no tint)
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
                    glBindTexture(GL_TEXTURE_2D, m_grassSideTexture);
                    chunk->RenderGrassFace(Chunk::GRASS_SIDE);
                    
                    // Render grass side overlay on top using polygon offset to avoid z-fighting
                    glEnable(GL_POLYGON_OFFSET_FILL);
                    glPolygonOffset(-1.0f, -1.0f); // Pull overlay slightly toward camera
                    glUniform3f(m_colorTintLoc, 0.8f, 0.3f, 0.8f); // Purple tint for overlay
                    glBindTexture(GL_TEXTURE_2D, m_grassSideOverlayTexture);
                    chunk->RenderGrassFace(Chunk::GRASS_SIDE);
                    glDisable(GL_POLYGON_OFFSET_FILL);
                    
                    // Render grass bottom faces (no tint) 
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
                    glBindTexture(GL_TEXTURE_2D, m_grassBottomTexture);
                    chunk->RenderGrassFace(Chunk::GRASS_BOTTOM);
                }
            }
        } else {
            // Handle non-grass blocks (stone, dirt, etc.)
            // Render only the center chunk (0,0) for static camera mode
            int x = 3, z = 3;
            int chunkX = x - 3; // = 0
            int chunkZ = z - 3; // = 0
            const Chunk* chunk = world.GetChunk(chunkX, chunkZ);
            
            if (chunk && chunk->HasMesh()) {
                if (m_renderMode == RenderMode::WHITE_ONLY) {
                    // White only: render with white color, no textures
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f);
                    chunk->RenderMeshForBlockType(blockType);
                } else if (m_renderMode == RenderMode::AO_ONLY) {
                    // AO only: render with AO but no textures
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); 
                    chunk->RenderMeshForBlockType(blockType);
                } else {
                    // Full render: normal texture rendering
                    glUniform3f(m_colorTintLoc, 1.0f, 1.0f, 1.0f); // White (no tint)
                    if (static_cast<size_t>(blockType) < m_blockTextures.size() && m_blockTextures[static_cast<int>(blockType)] != 0) {
                        glBindTexture(GL_TEXTURE_2D, m_blockTextures[static_cast<int>(blockType)]);
                        chunk->RenderMeshForBlockType(blockType);
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
    
    float tanFov = tan(fov * M_PI / 360.0f); // fov/2 in radians
    
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
    
    std::cout << "Rendering " << otherPlayers.size() << " other players:" << std::endl;
    
    // Set the view and projection matrices for player rendering
    glUseProgram(m_playerShaderProgram);
    glUniformMatrix4fv(m_playerViewLoc, 1, GL_FALSE, m_viewMatrix.m);
    glUniformMatrix4fv(m_playerProjLoc, 1, GL_FALSE, m_projectionMatrix.m);
    
    // Render each other player with the Minecraft-like player model
    for (const auto& pair : otherPlayers) {
        const PlayerPosition& playerPos = pair.second;
        
        std::cout << "  Player " << pair.first << " at (" << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ")" << std::endl;
        
        // Render player model at their position with their rotation
        Vec3 position(playerPos.x, playerPos.y, playerPos.z);
        
        m_playerModel.Render(position, playerPos.yaw, playerPos.pitch);
    }
    
    std::cout << "Player rendering complete." << std::endl;
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
    // Initialize texture array with size for our block types
    m_blockTextures.resize(4); // AIR, STONE, DIRT, GRASS
    
    // Load stone texture (BlockType::STONE = 1)
    m_blockTextures[1] = LoadTexture("assets/block/stone.png");
    
    // Load dirt texture (BlockType::DIRT = 2)  
    m_blockTextures[2] = LoadTexture("assets/block/dirt.png");
    
    // Load grass textures (BlockType::GRASS = 3)
    m_grassTopTexture = LoadTexture("assets/block/grass_block_top.png");
    m_grassSideTexture = LoadTexture("assets/block/grass_block_side.png");
    // Force overlay texture to load as RGBA for proper alpha handling
    m_grassSideOverlayTexture = LoadTextureWithAlpha("assets/block/grass_block_side_overlay.png");
    m_grassBottomTexture = LoadTexture("assets/block/dirt.png"); // Use dirt texture for grass bottom
    
    // For now, set grass block texture to side texture (will be overridden per face)
    m_blockTextures[3] = m_grassSideTexture;
    
    // AIR doesn't need a texture (index 0 can remain 0)
    m_blockTextures[0] = 0;
    
    return true;
} 