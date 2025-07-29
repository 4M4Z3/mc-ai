#include "Renderer.h"
#include "World.h"
#include "Chunk.h"
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>

Renderer::Renderer() : m_cubeVAO(0), m_cubeVBO(0), m_shaderProgram(0), 
                       m_triangleVAO(0), m_triangleVBO(0),
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

    // Get uniform locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projLoc = glGetUniformLocation(m_shaderProgram, "projection");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable backface culling to hide faces facing away from camera
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Counter-clockwise winding for front faces
    
    // Set initial viewport and projection
    SetViewport(m_viewportWidth, m_viewportHeight);

    std::cout << "3D Renderer initialized successfully!" << std::endl;
    return true;
}

bool Renderer::CreateCubeGeometry() {
    // Cube vertices (positions only)
    float cubeVertices[] = {
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

    glGenVertexArrays(1, &m_cubeVAO);
    glBindVertexArray(m_cubeVAO);

    glGenBuffers(1, &m_cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

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
    m_projectionMatrix = CreateProjectionMatrix(45.0f, aspect, 0.1f, 100.0f);
    
    std::cout << "Aspect ratio: " << aspect << std::endl;
}

void Renderer::BeginFrame(const Player& player) {
    // Ensure viewport is correct (in case ImGui or other code changed it)
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    glUseProgram(m_shaderProgram);
    
    // Set view matrix
    Mat4 viewMatrix = player.GetViewMatrix();
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, viewMatrix.m);
    
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
    
    // Render all chunks using their pre-generated meshes
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            const Chunk* chunk = nullptr;
            
            // Chunk coordinates now directly correspond to array indices
            int chunkX = x;
            int chunkZ = z;
            
            chunk = world.GetChunk(chunkX, chunkZ);
            if (chunk && chunk->HasMesh()) {
                chunk->RenderMesh();
            }
        }
    }
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
    // Render each other player as a colored cube at their position
    for (const auto& pair : otherPlayers) {
        const PlayerPosition& playerPos = pair.second;
        
        // Render player as a slightly larger cube (1.8 blocks tall like Minecraft players)
        // For now, render as a simple cube at their position
        RenderCube(playerPos.x, playerPos.y + 0.9f, playerPos.z); // Offset Y to center on player
    }
} 