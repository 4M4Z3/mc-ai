#pragma once

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <epoxy/gl.h>
#endif
#include <vector>
#include "Player.h"

class PlayerModel {
public:
    PlayerModel();
    ~PlayerModel();

    bool Initialize();
    void Shutdown();
    void Render(const Vec3& position, float yaw = 0.0f, float pitch = 0.0f);
    
    // Set shader program (to be called from Renderer)
    void UseShaderProgram(unsigned int shaderProgram);
    
    // Set uniform locations
    void SetUniformLocations(int modelLoc, int viewLoc, int projLoc);

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
    int m_modelLoc, m_viewLoc, m_projLoc;
    
    // Geometry generation
    void CreateHeadGeometry();
    void CreateTorsoGeometry();
    void CreateArmGeometry(unsigned int& vao, unsigned int& vbo);
    void CreateLegGeometry(unsigned int& vao, unsigned int& vbo);
    
    // Helper function to create cube geometry with specific dimensions
    std::vector<float> CreateCubeVertices(float width, float height, float depth, 
                                          float offsetX = 0.0f, float offsetY = 0.0f, float offsetZ = 0.0f);
    
    // Matrix operations
    Mat4 CreateScaleMatrix(float sx, float sy, float sz);
    Mat4 CreateRotationYMatrix(float angle);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
    Mat4 MultiplyMatrices(const Mat4& a, const Mat4& b);
}; 