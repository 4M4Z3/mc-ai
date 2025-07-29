#include "Renderer.h"
#include "World.h"
#include <iostream>
#include <string>
#include <cmath>

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
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // Sky blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
    
    // Update projection matrix
    float aspect = (float)width / (float)height;
    m_projectionMatrix = CreateProjectionMatrix(45.0f, aspect, 0.1f, 100.0f);
}

void Renderer::BeginFrame(const Player& player) {
    glUseProgram(m_shaderProgram);
    
    // Set view matrix
    Mat4 viewMatrix = player.GetViewMatrix();
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, viewMatrix.m);
    
    // Set projection matrix
    glUniformMatrix4fv(m_projLoc, 1, GL_FALSE, m_projectionMatrix.m);
}

void Renderer::RenderWorld(const World& world) {
    // Simple approach: check all possible positions in our small world
    for (int x = -16; x < 16; x++) {
        for (int y = 0; y < 10; y++) { // Only check lower part of world for performance
            for (int z = -16; z < 16; z++) {
                Block block = world.GetBlock(x, y, z);
                if (!block.IsAir()) {
                    RenderCube(x, y, z);
                }
            }
        }
    }
}

void Renderer::RenderCube(float x, float y, float z) {
    // Create translation matrix for this cube
    Mat4 modelMatrix = CreateTranslationMatrix(x, y, z);
    
    // Set model matrix uniform
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
    
    // Render cube
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::EndFrame() {
    // Nothing needed here for now
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

bool Renderer::CreateShaders() {
    // 3D Vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        void main()
        {
            FragColor = vec4(0.6f, 0.3f, 0.1f, 1.0f); // Brown block color
        }
    )";

    // Compile shaders
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
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