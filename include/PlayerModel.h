#pragma once

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <epoxy/gl.h>
#endif
#include <vector>
#include <string>
#include <random>
#include "Player.h"

class PlayerModel {
public:
    PlayerModel();
    ~PlayerModel();

    bool Initialize();
    void Shutdown();
    void Render(const Vec3& position, float yaw = 0.0f, float pitch = 0.0f);
    
    // First-person arm rendering (like Minecraft)
    void RenderFirstPersonArm(const Player& player);
    
    // First-person arm animation
    void TriggerPunchAnimation();
    void UpdateAnimation(float deltaTime);
    
    // Set shader program (to be called from Renderer)
    void UseShaderProgram(unsigned int shaderProgram);
    
    // Set uniform locations
    void SetUniformLocations(int modelLoc, int viewLoc, int projLoc, int skinTextureLoc);
    
    // Skin management
    bool LoadSkins();
    void AssignRandomSkin();
    void SetSkin(const std::string& skinName);

private:
    // OpenGL objects
    unsigned int m_headVAO, m_headVBO;
    unsigned int m_torsoVAO, m_torsoVBO;
    unsigned int m_leftArmVAO, m_leftArmVBO;
    unsigned int m_rightArmVAO, m_rightArmVBO;
    unsigned int m_leftLegVAO, m_leftLegVBO;
    unsigned int m_rightLegVAO, m_rightLegVBO;
    
    // Shader uniforms (set by Renderer)
    unsigned int m_shaderProgram;
    int m_modelLoc, m_viewLoc, m_projLoc, m_skinTextureLoc;
    
    // Skin management
    std::vector<std::string> m_availableSkins;
    std::vector<unsigned int> m_skinTextures;
    unsigned int m_currentSkinTexture;
    std::mt19937 m_randomGenerator;
    
    // First-person arm animation
    bool m_isPunching;
    float m_punchAnimationTime;
    float m_punchAnimationDuration;
    
    // Geometry generation
    void CreateHeadGeometry();
    void CreateTorsoGeometry();
    void CreateArmGeometry(unsigned int& vao, unsigned int& vbo, const std::vector<std::vector<float>>& uvMapping);
    void CreateLegGeometry(unsigned int& vao, unsigned int& vbo, const std::vector<std::vector<float>>& uvMapping);
    
    // Helper function to create cube geometry with specific dimensions and UV coordinates
    std::vector<float> CreateCubeVerticesWithUV(float width, float height, float depth, 
                                                float offsetX = 0.0f, float offsetY = 0.0f, float offsetZ = 0.0f,
                                                const std::vector<std::vector<float>>& faceUVs = {});
    
    // UV mapping for Minecraft skin format
    std::vector<std::vector<float>> GetHeadUVMapping();
    std::vector<std::vector<float>> GetTorsoUVMapping();
    std::vector<std::vector<float>> GetRightArmUVMapping();
    std::vector<std::vector<float>> GetLeftArmUVMapping();
    std::vector<std::vector<float>> GetRightLegUVMapping();
    std::vector<std::vector<float>> GetLeftLegUVMapping();
    
    // Utility functions
    unsigned int LoadSkinTexture(const std::string& skinPath);
    
    // Matrix operations
    Mat4 CreateScaleMatrix(float sx, float sy, float sz);
    Mat4 CreateRotationYMatrix(float angle);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
    Mat4 MultiplyMatrices(const Mat4& a, const Mat4& b);
}; 