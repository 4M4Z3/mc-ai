#include "PlayerModel.h"
#include <iostream>
#include <cmath>

PlayerModel::PlayerModel() : 
    m_headVAO(0), m_headVBO(0),
    m_torsoVAO(0), m_torsoVBO(0),
    m_leftArmVAO(0), m_leftArmVBO(0),
    m_rightArmVAO(0), m_rightArmVBO(0),
    m_leftLegVAO(0), m_leftLegVBO(0),
    m_rightLegVAO(0), m_rightLegVBO(0),
    m_shaderProgram(0),
    m_modelLoc(-1), m_viewLoc(-1), m_projLoc(-1) {
}

PlayerModel::~PlayerModel() {
    Shutdown();
}

bool PlayerModel::Initialize() {
    CreateHeadGeometry();
    CreateTorsoGeometry();
    CreateArmGeometry(m_leftArmVAO, m_leftArmVBO);
    CreateArmGeometry(m_rightArmVAO, m_rightArmVBO);
    CreateLegGeometry(m_leftLegVAO, m_leftLegVBO);
    CreateLegGeometry(m_rightLegVAO, m_rightLegVBO);
    
    return true;
}

void PlayerModel::Shutdown() {
    // Clean up VAOs and VBOs
    if (m_headVAO) { glDeleteVertexArrays(1, &m_headVAO); m_headVAO = 0; }
    if (m_headVBO) { glDeleteBuffers(1, &m_headVBO); m_headVBO = 0; }
    if (m_torsoVAO) { glDeleteVertexArrays(1, &m_torsoVAO); m_torsoVAO = 0; }
    if (m_torsoVBO) { glDeleteBuffers(1, &m_torsoVBO); m_torsoVBO = 0; }
    if (m_leftArmVAO) { glDeleteVertexArrays(1, &m_leftArmVAO); m_leftArmVAO = 0; }
    if (m_leftArmVBO) { glDeleteBuffers(1, &m_leftArmVBO); m_leftArmVBO = 0; }
    if (m_rightArmVAO) { glDeleteVertexArrays(1, &m_rightArmVAO); m_rightArmVAO = 0; }
    if (m_rightArmVBO) { glDeleteBuffers(1, &m_rightArmVBO); m_rightArmVBO = 0; }
    if (m_leftLegVAO) { glDeleteVertexArrays(1, &m_leftLegVAO); m_leftLegVAO = 0; }
    if (m_leftLegVBO) { glDeleteBuffers(1, &m_leftLegVBO); m_leftLegVBO = 0; }
    if (m_rightLegVAO) { glDeleteVertexArrays(1, &m_rightLegVAO); m_rightLegVAO = 0; }
    if (m_rightLegVBO) { glDeleteBuffers(1, &m_rightLegVBO); m_rightLegVBO = 0; }
}

void PlayerModel::UseShaderProgram(unsigned int shaderProgram) {
    m_shaderProgram = shaderProgram;
}

void PlayerModel::SetUniformLocations(int modelLoc, int viewLoc, int projLoc) {
    m_modelLoc = modelLoc;
    m_viewLoc = viewLoc;
    m_projLoc = projLoc;
}

void PlayerModel::Render(const Vec3& position, float yaw, float pitch) {
    if (m_shaderProgram == 0) return;
    
    glUseProgram(m_shaderProgram);
    
    // Base transformation matrix for the entire player
    Mat4 baseTransform = CreateTranslationMatrix(position.x, position.y, position.z);
    Mat4 yawRotation = CreateRotationYMatrix(yaw * M_PI / 180.0f);
    Mat4 playerTransform = MultiplyMatrices(baseTransform, yawRotation);
    
    // Render Head (0.5x0.5x0.5 blocks at top)
    {
        Mat4 headTransform = CreateTranslationMatrix(0.0f, 1.55f, 0.0f); // 1.8 - 0.25 (half head height)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, headTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_headVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 6 faces * 6 vertices
    }
    
    // Render Torso (0.5x0.75x0.25 blocks)
    {
        Mat4 torsoTransform = CreateTranslationMatrix(0.0f, 1.0f, 0.0f); // Center at 1.0 blocks high
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, torsoTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_torsoVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Left Arm (0.25x0.75x0.25 blocks)
    {
        Mat4 armTransform = CreateTranslationMatrix(-0.375f, 1.0f, 0.0f); // -0.25 (torso half width) - 0.125 (arm half width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, armTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_leftArmVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Right Arm (0.25x0.75x0.25 blocks)
    {
        Mat4 armTransform = CreateTranslationMatrix(0.375f, 1.0f, 0.0f); // 0.25 (torso half width) + 0.125 (arm half width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, armTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_rightArmVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Left Leg (0.25x0.75x0.25 blocks)
    {
        Mat4 legTransform = CreateTranslationMatrix(-0.125f, 0.375f, 0.0f); // -0.125 (quarter torso width), 0.375 (half leg height)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, legTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_leftLegVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Right Leg (0.25x0.75x0.25 blocks)
    {
        Mat4 legTransform = CreateTranslationMatrix(0.125f, 0.375f, 0.0f); // 0.125 (quarter torso width), 0.375 (half leg height)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, legTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_rightLegVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    glBindVertexArray(0);
}

void PlayerModel::CreateHeadGeometry() {
    // Head: 0.5x0.5x0.5 blocks (cube)
    std::vector<float> vertices = CreateCubeVertices(0.5f, 0.5f, 0.5f);
    
    glGenVertexArrays(1, &m_headVAO);
    glBindVertexArray(m_headVAO);
    
    glGenBuffers(1, &m_headVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_headVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateTorsoGeometry() {
    // Torso: 0.5x0.75x0.25 blocks
    std::vector<float> vertices = CreateCubeVertices(0.5f, 0.75f, 0.25f);
    
    glGenVertexArrays(1, &m_torsoVAO);
    glBindVertexArray(m_torsoVAO);
    
    glGenBuffers(1, &m_torsoVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_torsoVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateArmGeometry(unsigned int& vao, unsigned int& vbo) {
    // Arms: 0.25x0.75x0.25 blocks
    std::vector<float> vertices = CreateCubeVertices(0.25f, 0.75f, 0.25f);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateLegGeometry(unsigned int& vao, unsigned int& vbo) {
    // Legs: 0.25x0.75x0.25 blocks
    std::vector<float> vertices = CreateCubeVertices(0.25f, 0.75f, 0.25f);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::vector<float> PlayerModel::CreateCubeVertices(float width, float height, float depth, 
                                                   float offsetX, float offsetY, float offsetZ) {
    float hw = width * 0.5f;   // half width
    float hh = height * 0.5f;  // half height
    float hd = depth * 0.5f;   // half depth
    
    // Cube vertices (6 faces, 2 triangles per face, 3 vertices per triangle)
    std::vector<float> vertices = {
        // Front face
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,
         hw + offsetX, -hh + offsetY,  hd + offsetZ,
         hw + offsetX,  hh + offsetY,  hd + offsetZ,
         hw + offsetX,  hh + offsetY,  hd + offsetZ,
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,

        // Back face
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX,  hh + offsetY, -hd + offsetZ,
         hw + offsetX,  hh + offsetY, -hd + offsetZ,
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,

        // Left face
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,

        // Right face
         hw + offsetX,  hh + offsetY,  hd + offsetZ,
         hw + offsetX,  hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY,  hd + offsetZ,
         hw + offsetX,  hh + offsetY,  hd + offsetZ,

        // Bottom face
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY, -hd + offsetZ,
         hw + offsetX, -hh + offsetY,  hd + offsetZ,
         hw + offsetX, -hh + offsetY,  hd + offsetZ,
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,

        // Top face
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,
         hw + offsetX,  hh + offsetY, -hd + offsetZ,
         hw + offsetX,  hh + offsetY,  hd + offsetZ,
         hw + offsetX,  hh + offsetY,  hd + offsetZ,
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,
        -hw + offsetX,  hh + offsetY, -hd + offsetZ
    };
    
    return vertices;
}

Mat4 PlayerModel::CreateScaleMatrix(float sx, float sy, float sz) {
    Mat4 scale;
    scale.m[0] = sx;
    scale.m[5] = sy;
    scale.m[10] = sz;
    return scale;
}

Mat4 PlayerModel::CreateRotationYMatrix(float angle) {
    Mat4 rot;
    float c = cos(angle);
    float s = sin(angle);
    
    rot.m[0] = c;
    rot.m[2] = s;
    rot.m[8] = -s;
    rot.m[10] = c;
    
    return rot;
}

Mat4 PlayerModel::CreateTranslationMatrix(float x, float y, float z) {
    Mat4 trans;
    trans.m[12] = x;
    trans.m[13] = y;
    trans.m[14] = z;
    return trans;
}

Mat4 PlayerModel::MultiplyMatrices(const Mat4& a, const Mat4& b) {
    Mat4 result;
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    
    return result;
} 