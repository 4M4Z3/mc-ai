#include "PlayerModel.h"
#include <iostream>
#include <cmath>  // For M_PI and fmod
#include <filesystem>
#include <random>
#include <chrono>
#include "../third_party/stb_image.h"

PlayerModel::PlayerModel() : 
    m_headVAO(0), m_headVBO(0),
    m_torsoVAO(0), m_torsoVBO(0),
    m_leftArmVAO(0), m_leftArmVBO(0),
    m_rightArmVAO(0), m_rightArmVBO(0),
    m_leftLegVAO(0), m_leftLegVBO(0),
    m_rightLegVAO(0), m_rightLegVBO(0),
    m_shaderProgram(0),
    m_modelLoc(-1), m_viewLoc(-1), m_projLoc(-1), m_skinTextureLoc(-1),
    m_currentSkinTexture(0),
    m_randomGenerator(std::chrono::steady_clock::now().time_since_epoch().count()) {
}

PlayerModel::~PlayerModel() {
    Shutdown();
}

bool PlayerModel::Initialize() {
    // Load available skins
    if (!LoadSkins()) {
        std::cerr << "Failed to load skins" << std::endl;
        return false;
    }
    
    // Assign random skin
    AssignRandomSkin();
    
    CreateHeadGeometry();
    CreateTorsoGeometry();
    CreateArmGeometry(m_leftArmVAO, m_leftArmVBO, GetLeftArmUVMapping());
    CreateArmGeometry(m_rightArmVAO, m_rightArmVBO, GetRightArmUVMapping());
    CreateLegGeometry(m_leftLegVAO, m_leftLegVBO, GetLeftLegUVMapping());
    CreateLegGeometry(m_rightLegVAO, m_rightLegVBO, GetRightLegUVMapping());
    
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
    
    // Clean up skin textures
    for (unsigned int texture : m_skinTextures) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    m_skinTextures.clear();
}

void PlayerModel::UseShaderProgram(unsigned int shaderProgram) {
    m_shaderProgram = shaderProgram;
}

void PlayerModel::SetUniformLocations(int modelLoc, int viewLoc, int projLoc, int skinTextureLoc) {
    m_modelLoc = modelLoc;
    m_viewLoc = viewLoc;
    m_projLoc = projLoc;
    m_skinTextureLoc = skinTextureLoc;
}

void PlayerModel::Render(const Vec3& position, float yaw, float pitch) {
    if (m_shaderProgram == 0) return;
    
    // Don't switch shader programs - use the one set by Renderer
    // glUseProgram(m_shaderProgram);
    
    // Bind the current skin texture
    if (m_currentSkinTexture != 0 && m_skinTextureLoc != -1) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_currentSkinTexture);
        glUniform1i(m_skinTextureLoc, 0);
    }
    
    // Normalize yaw to prevent accumulation issues
    float normalizedYaw = fmod(yaw, 360.0f);
    if (normalizedYaw < 0.0f) normalizedYaw += 360.0f;
    
    // Base transformation matrix for the entire player
    // Player position represents center at ground level (feet)
    // Total player height is 1.8 blocks
    // Apply translation first, then rotation, so player rotates around their own center
    Mat4 translation = CreateTranslationMatrix(position.x, position.y, position.z);
    Mat4 yawRotation = CreateRotationYMatrix(normalizedYaw * M_PI / 180.0f);
    Mat4 playerTransform = MultiplyMatrices(yawRotation, translation);
    
    // Render Head (0.5x0.45x0.5 blocks) - top of player
    // Position: y + 1.575 (center of head at 1.575 blocks above feet)
    {
        Mat4 headTransform = CreateTranslationMatrix(0.0f, 1.575f, 0.0f);
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, headTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_headVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 6 faces * 6 vertices
    }
    
    // Render Torso (0.5x0.675x0.25 blocks) - middle of player
    // Position: y + 1.0125 (center of torso at 1.0125 blocks above feet)
    {
        Mat4 torsoTransform = CreateTranslationMatrix(0.0f, 1.0125f, 0.0f);
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, torsoTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_torsoVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Left Arm (0.25x0.675x0.25 blocks) - attached to torso
    // Position: align with torso center at y + 1.0125
    {
        Mat4 armTransform = CreateTranslationMatrix(-0.375f, 1.0125f, 0.0f); // -0.25 (torso half width) - 0.125 (arm half width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, armTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_leftArmVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Right Arm (0.25x0.675x0.25 blocks) - attached to torso
    // Position: align with torso center at y + 1.0125
    {
        Mat4 armTransform = CreateTranslationMatrix(0.375f, 1.0125f, 0.0f); // 0.25 (torso half width) + 0.125 (arm half width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, armTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_rightArmVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Left Leg (0.25x0.675x0.25 blocks) - bottom of player
    // Position: y + 0.3375 (center of leg at 0.3375 blocks above feet)
    {
        Mat4 legTransform = CreateTranslationMatrix(-0.125f, 0.3375f, 0.0f); // -0.125 (quarter torso width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, legTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_leftLegVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Render Right Leg (0.25x0.675x0.25 blocks) - bottom of player
    // Position: y + 0.3375 (center of leg at 0.3375 blocks above feet)
    {
        Mat4 legTransform = CreateTranslationMatrix(0.125f, 0.3375f, 0.0f); // 0.125 (quarter torso width)
        Mat4 modelMatrix = MultiplyMatrices(playerTransform, legTransform);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, modelMatrix.m);
        
        glBindVertexArray(m_rightLegVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    glBindVertexArray(0);
}

void PlayerModel::CreateHeadGeometry() {
    // Head: 0.5x0.45x0.5 blocks (slightly shorter cube)
    std::vector<float> vertices = CreateCubeVerticesWithUV(0.5f, 0.45f, 0.5f, 0.0f, 0.0f, 0.0f, GetHeadUVMapping());
    
    glGenVertexArrays(1, &m_headVAO);
    glBindVertexArray(m_headVAO);
    
    glGenBuffers(1, &m_headVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_headVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateTorsoGeometry() {
    // Torso: 0.5x0.675x0.25 blocks
    std::vector<float> vertices = CreateCubeVerticesWithUV(0.5f, 0.675f, 0.25f, 0.0f, 0.0f, 0.0f, GetTorsoUVMapping());
    
    glGenVertexArrays(1, &m_torsoVAO);
    glBindVertexArray(m_torsoVAO);
    
    glGenBuffers(1, &m_torsoVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_torsoVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateArmGeometry(unsigned int& vao, unsigned int& vbo, const std::vector<std::vector<float>>& uvMapping) {
    // Arms: 0.25x0.675x0.25 blocks
    std::vector<float> vertices = CreateCubeVerticesWithUV(0.25f, 0.675f, 0.25f, 0.0f, 0.0f, 0.0f, uvMapping);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlayerModel::CreateLegGeometry(unsigned int& vao, unsigned int& vbo, const std::vector<std::vector<float>>& uvMapping) {
    // Legs: 0.25x0.675x0.25 blocks
    std::vector<float> vertices = CreateCubeVerticesWithUV(0.25f, 0.675f, 0.25f, 0.0f, 0.0f, 0.0f, uvMapping);
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}



// Helper function to create cube geometry with UV coordinates
std::vector<float> PlayerModel::CreateCubeVerticesWithUV(float width, float height, float depth, 
                                                          float offsetX, float offsetY, float offsetZ,
                                                          const std::vector<std::vector<float>>& faceUVs) {
    float hw = width * 0.5f;   // half width
    float hh = height * 0.5f;  // half height
    float hd = depth * 0.5f;   // half depth
    
    // Default UV coordinates if none provided (maps to full texture)
    std::vector<std::vector<float>> defaultUVs = {
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, // Front
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, // Back
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, // Left
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, // Right
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, // Bottom
        {0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}  // Top
    };
    
    const auto& uvs = faceUVs.empty() ? defaultUVs : faceUVs;
    
    // Cube vertices with UV coordinates (5 floats per vertex: x, y, z, u, v)
    // Face order: Front, Back, Left, Right, Bottom, Top
    std::vector<float> vertices = {
        // Front face (+Z)
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[0][0], uvs[0][1],   // bottom-left
         hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[0][2], uvs[0][3],   // bottom-right
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[0][4], uvs[0][5],   // top-right
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[0][6], uvs[0][7],   // top-right
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[0][8], uvs[0][9],   // top-left
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[0][10], uvs[0][11], // bottom-left

        // Back face (-Z)
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[1][0], uvs[1][1],
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[1][8], uvs[1][9],
         hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[1][4], uvs[1][5],
         hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[1][6], uvs[1][7],
         hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[1][2], uvs[1][3],
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[1][10], uvs[1][11],

        // Left face (-X)
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[2][8], uvs[2][9],
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[2][4], uvs[2][5],
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[2][2], uvs[2][3],
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[2][6], uvs[2][7],
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[2][0], uvs[2][1],
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[2][10], uvs[2][11],

        // Right face (+X)
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[3][8], uvs[3][9],
         hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[3][0], uvs[3][1],
         hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[3][2], uvs[3][3],
         hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[3][6], uvs[3][7],
         hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[3][4], uvs[3][5],
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[3][10], uvs[3][11],

        // Bottom face (-Y)
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[4][0], uvs[4][1],
         hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[4][2], uvs[4][3],
         hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[4][4], uvs[4][5],
         hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[4][6], uvs[4][7],
        -hw + offsetX, -hh + offsetY,  hd + offsetZ,  uvs[4][8], uvs[4][9],
        -hw + offsetX, -hh + offsetY, -hd + offsetZ,  uvs[4][10], uvs[4][11],

        // Top face (+Y)
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[5][0], uvs[5][1],
        -hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[5][8], uvs[5][9],
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[5][4], uvs[5][5],
         hw + offsetX,  hh + offsetY,  hd + offsetZ,  uvs[5][6], uvs[5][7],
         hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[5][2], uvs[5][3],
        -hw + offsetX,  hh + offsetY, -hd + offsetZ,  uvs[5][10], uvs[5][11]
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

// Skin management methods
bool PlayerModel::LoadSkins() {
    std::string skinsPath = "assets/skins";
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(skinsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                std::string skinName = entry.path().stem().string();
                std::string skinPath = entry.path().string();
                
                unsigned int textureID = LoadSkinTexture(skinPath);
                if (textureID != 0) {
                    m_availableSkins.push_back(skinName);
                    m_skinTextures.push_back(textureID);
                    std::cout << "Loaded skin: " << skinName << std::endl;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing skins directory: " << e.what() << std::endl;
        return false;
    }
    
    if (m_availableSkins.empty()) {
        std::cerr << "No valid skins found in " << skinsPath << std::endl;
        return false;
    }
    
    std::cout << "Loaded " << m_availableSkins.size() << " skins" << std::endl;
    return true;
}

void PlayerModel::AssignRandomSkin() {
    if (m_availableSkins.empty()) return;
    
    std::uniform_int_distribution<size_t> dist(0, m_availableSkins.size() - 1);
    size_t randomIndex = dist(m_randomGenerator);
    
    m_currentSkinTexture = m_skinTextures[randomIndex];
    std::cout << "Assigned random skin: " << m_availableSkins[randomIndex] << std::endl;
}

void PlayerModel::SetSkin(const std::string& skinName) {
    for (size_t i = 0; i < m_availableSkins.size(); ++i) {
        if (m_availableSkins[i] == skinName) {
            m_currentSkinTexture = m_skinTextures[i];
            std::cout << "Set skin to: " << skinName << std::endl;
            return;
        }
    }
    std::cerr << "Skin not found: " << skinName << std::endl;
}

unsigned int PlayerModel::LoadSkinTexture(const std::string& skinPath) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters for pixel art
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Load image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(skinPath.c_str(), &width, &height, &nrChannels, 4); // Force RGBA
    
    if (data) {
        if (width != 64 || height != 64) {
            std::cerr << "Warning: Skin " << skinPath << " is not 64x64 pixels (" << width << "x" << height << ")" << std::endl;
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Loaded skin texture: " << skinPath << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cerr << "Failed to load skin texture: " << skinPath << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
} 

// UV mapping for Minecraft skin format (64x64, each section is 8x8 pixels = 0.125 texture units)
std::vector<std::vector<float>> PlayerModel::GetHeadUVMapping() {
    // Head mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (head front) - (8, 8) to (16, 16)
        {0.125f, 0.25f,  0.25f, 0.25f,  0.25f, 0.125f,  0.25f, 0.125f,  0.125f, 0.125f,  0.125f, 0.25f},
        // Back face (head back) - (24, 8) to (32, 16) - flipped horizontally
        {0.5f, 0.25f,  0.375f, 0.25f,  0.375f, 0.125f,  0.375f, 0.125f,  0.5f, 0.125f,  0.5f, 0.25f},
        // Left face (head right texture) - (0, 8) to (8, 16) (player's right side appears on left)
        {0.0f, 0.25f,  0.125f, 0.25f,  0.125f, 0.125f,  0.125f, 0.125f,  0.0f, 0.125f,  0.0f, 0.25f},
        // Right face (head left texture) - (16, 8) to (24, 16) (player's left side appears on right)  
        {0.375f, 0.25f,  0.25f, 0.25f,  0.25f, 0.125f,  0.25f, 0.125f,  0.375f, 0.125f,  0.375f, 0.25f},
        // Bottom face (head bottom) - (16, 0) to (24, 8)
        {0.25f, 0.125f,  0.375f, 0.125f,  0.375f, 0.0f,  0.375f, 0.0f,  0.25f, 0.0f,  0.25f, 0.125f},
        // Top face (head top) - (8, 0) to (16, 8)
        {0.125f, 0.125f,  0.25f, 0.125f,  0.25f, 0.0f,  0.25f, 0.0f,  0.125f, 0.0f,  0.125f, 0.125f}
    };
}

std::vector<std::vector<float>> PlayerModel::GetTorsoUVMapping() {
    // Torso mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (torso front) - (20, 20) to (28, 32)
        {0.3125f, 0.5f,  0.4375f, 0.5f,  0.4375f, 0.3125f,  0.4375f, 0.3125f,  0.3125f, 0.3125f,  0.3125f, 0.5f},
        // Back face (torso back) - (32, 20) to (40, 32) - flipped horizontally
        {0.625f, 0.5f,  0.5f, 0.5f,  0.5f, 0.3125f,  0.5f, 0.3125f,  0.625f, 0.3125f,  0.625f, 0.5f},
        // Left face (torso right texture) - (16, 20) to (20, 32) (player's right side appears on left)
        {0.25f, 0.5f,  0.3125f, 0.5f,  0.3125f, 0.3125f,  0.3125f, 0.3125f,  0.25f, 0.3125f,  0.25f, 0.5f},
        // Right face (torso left texture) - (28, 20) to (32, 32) (player's left side appears on right)
        {0.5f, 0.5f,  0.4375f, 0.5f,  0.4375f, 0.3125f,  0.4375f, 0.3125f,  0.5f, 0.3125f,  0.5f, 0.5f},
        // Bottom face (torso bottom) - (28, 16) to (36, 20)
        {0.4375f, 0.3125f,  0.5625f, 0.3125f,  0.5625f, 0.25f,  0.5625f, 0.25f,  0.4375f, 0.25f,  0.4375f, 0.3125f},
        // Top face (torso top) - (20, 16) to (28, 20)
        {0.3125f, 0.3125f,  0.4375f, 0.3125f,  0.4375f, 0.25f,  0.4375f, 0.25f,  0.3125f, 0.25f,  0.3125f, 0.3125f}
    };
}

std::vector<std::vector<float>> PlayerModel::GetRightArmUVMapping() {
    // Right arm mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (right arm front) - (44, 20) to (48, 32)
        {0.6875f, 0.5f,  0.75f, 0.5f,  0.75f, 0.3125f,  0.75f, 0.3125f,  0.6875f, 0.3125f,  0.6875f, 0.5f},
        // Back face (right arm back) - (52, 20) to (56, 32) - flipped horizontally
        {0.875f, 0.5f,  0.8125f, 0.5f,  0.8125f, 0.3125f,  0.8125f, 0.3125f,  0.875f, 0.3125f,  0.875f, 0.5f},
        // Left face (right arm right texture) - (40, 20) to (44, 32) (arm's right side appears on left)
        {0.625f, 0.5f,  0.6875f, 0.5f,  0.6875f, 0.3125f,  0.6875f, 0.3125f,  0.625f, 0.3125f,  0.625f, 0.5f},
        // Right face (right arm left texture) - (48, 20) to (52, 32) (arm's left side appears on right)
        {0.8125f, 0.5f,  0.75f, 0.5f,  0.75f, 0.3125f,  0.75f, 0.3125f,  0.8125f, 0.3125f,  0.8125f, 0.5f},
        // Bottom face (right arm bottom) - (48, 16) to (52, 20)
        {0.75f, 0.3125f,  0.8125f, 0.3125f,  0.8125f, 0.25f,  0.8125f, 0.25f,  0.75f, 0.25f,  0.75f, 0.3125f},
        // Top face (right arm top) - (44, 16) to (48, 20)
        {0.6875f, 0.3125f,  0.75f, 0.3125f,  0.75f, 0.25f,  0.75f, 0.25f,  0.6875f, 0.25f,  0.6875f, 0.3125f}
    };
}

std::vector<std::vector<float>> PlayerModel::GetLeftArmUVMapping() {
    // Left arm mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (left arm front) - (36, 52) to (40, 64)
        {0.5625f, 1.0f,  0.625f, 1.0f,  0.625f, 0.8125f,  0.625f, 0.8125f,  0.5625f, 0.8125f,  0.5625f, 1.0f},
        // Back face (left arm back) - (44, 52) to (48, 64) - flipped horizontally
        {0.75f, 1.0f,  0.6875f, 1.0f,  0.6875f, 0.8125f,  0.6875f, 0.8125f,  0.75f, 0.8125f,  0.75f, 1.0f},
        // Left face (left arm right texture) - (32, 52) to (36, 64) (arm's right side appears on left)
        {0.5f, 1.0f,  0.5625f, 1.0f,  0.5625f, 0.8125f,  0.5625f, 0.8125f,  0.5f, 0.8125f,  0.5f, 1.0f},
        // Right face (left arm left texture) - (40, 52) to (44, 64) (arm's left side appears on right)
        {0.6875f, 1.0f,  0.625f, 1.0f,  0.625f, 0.8125f,  0.625f, 0.8125f,  0.6875f, 0.8125f,  0.6875f, 1.0f},
        // Bottom face (left arm bottom) - (40, 48) to (44, 52)
        {0.625f, 0.8125f,  0.6875f, 0.8125f,  0.6875f, 0.75f,  0.6875f, 0.75f,  0.625f, 0.75f,  0.625f, 0.8125f},
        // Top face (left arm top) - (36, 48) to (40, 52)
        {0.5625f, 0.8125f,  0.625f, 0.8125f,  0.625f, 0.75f,  0.625f, 0.75f,  0.5625f, 0.75f,  0.5625f, 0.8125f}
    };
}

std::vector<std::vector<float>> PlayerModel::GetRightLegUVMapping() {
    // Right leg mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (right leg front) - (4, 20) to (8, 32)
        {0.0625f, 0.5f,  0.125f, 0.5f,  0.125f, 0.3125f,  0.125f, 0.3125f,  0.0625f, 0.3125f,  0.0625f, 0.5f},
        // Back face (right leg back) - (12, 20) to (16, 32) - flipped horizontally
        {0.25f, 0.5f,  0.1875f, 0.5f,  0.1875f, 0.3125f,  0.1875f, 0.3125f,  0.25f, 0.3125f,  0.25f, 0.5f},
        // Left face (right leg right texture) - (0, 20) to (4, 32) (leg's right side appears on left)
        {0.0f, 0.5f,  0.0625f, 0.5f,  0.0625f, 0.3125f,  0.0625f, 0.3125f,  0.0f, 0.3125f,  0.0f, 0.5f},
        // Right face (right leg left texture) - (8, 20) to (12, 32) (leg's left side appears on right)
        {0.1875f, 0.5f,  0.125f, 0.5f,  0.125f, 0.3125f,  0.125f, 0.3125f,  0.1875f, 0.3125f,  0.1875f, 0.5f},
        // Bottom face (right leg bottom) - (8, 16) to (12, 20)
        {0.125f, 0.3125f,  0.1875f, 0.3125f,  0.1875f, 0.25f,  0.1875f, 0.25f,  0.125f, 0.25f,  0.125f, 0.3125f},
        // Top face (right leg top) - (4, 16) to (8, 20)
        {0.0625f, 0.3125f,  0.125f, 0.3125f,  0.125f, 0.25f,  0.125f, 0.25f,  0.0625f, 0.25f,  0.0625f, 0.3125f}
    };
}

std::vector<std::vector<float>> PlayerModel::GetLeftLegUVMapping() {
    // Left leg mapping: front, back, left, right, bottom, top
    // Face order matches CreateCubeVerticesWithUV: Front, Back, Left, Right, Bottom, Top
    // UV coordinates: front-bottom, back-bottom, back-top, back-top, front-top, front-bottom (for side faces)
    return {
        // Front face (left leg front) - (20, 52) to (24, 64)
        {0.3125f, 1.0f,  0.375f, 1.0f,  0.375f, 0.8125f,  0.375f, 0.8125f,  0.3125f, 0.8125f,  0.3125f, 1.0f},
        // Back face (left leg back) - (28, 52) to (32, 64) - flipped horizontally
        {0.5f, 1.0f,  0.4375f, 1.0f,  0.4375f, 0.8125f,  0.4375f, 0.8125f,  0.5f, 0.8125f,  0.5f, 1.0f},
        // Left face (left leg right texture) - (16, 52) to (20, 64) (leg's right side appears on left)
        {0.25f, 1.0f,  0.3125f, 1.0f,  0.3125f, 0.8125f,  0.3125f, 0.8125f,  0.25f, 0.8125f,  0.25f, 1.0f},
        // Right face (left leg left texture) - (24, 52) to (28, 64) (leg's left side appears on right)
        {0.4375f, 1.0f,  0.375f, 1.0f,  0.375f, 0.8125f,  0.375f, 0.8125f,  0.4375f, 0.8125f,  0.4375f, 1.0f},
        // Bottom face (left leg bottom) - (24, 48) to (28, 52)
        {0.375f, 0.8125f,  0.4375f, 0.8125f,  0.4375f, 0.75f,  0.4375f, 0.75f,  0.375f, 0.75f,  0.375f, 0.8125f},
        // Top face (left leg top) - (20, 48) to (24, 52)
        {0.3125f, 0.8125f,  0.375f, 0.8125f,  0.375f, 0.75f,  0.375f, 0.75f,  0.3125f, 0.75f,  0.3125f, 0.8125f}
    };
} 