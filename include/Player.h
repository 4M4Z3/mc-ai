#pragma once

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

// Simple 3D vector and matrix structures
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
};

struct Mat4 {
    float m[16];
    
    Mat4() {
        // Initialize as identity matrix
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
};

class Player {
public:
    Player();
    Player(float x, float y, float z);
    
    // Position and rotation
    Vec3 GetPosition() const { return m_position; }
    void SetPosition(const Vec3& position) { m_position = position; }
    void SetPosition(float x, float y, float z) { m_position = Vec3(x, y, z); }
    
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    void SetRotation(float yaw, float pitch);
    
    // Movement
    void MoveForward(float distance);
    void MoveBackward(float distance);
    void MoveLeft(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void MoveDown(float distance);
    
    // Mouse look
    void ProcessMouseMovement(float xOffset, float yOffset, float sensitivity = 0.1f);
    
    // Input processing
    void ProcessInput(GLFWwindow* window, float deltaTime);
    
    // Camera matrices
    Mat4 GetViewMatrix() const;
    Vec3 GetForwardVector() const;
    Vec3 GetRightVector() const;
    Vec3 GetUpVector() const;

private:
    Vec3 m_position;
    float m_yaw;    // Rotation around Y axis
    float m_pitch;  // Rotation around X axis
    
    // Movement speed
    float m_movementSpeed;
    
    // Helper functions
    void UpdateVectors();
    Vec3 m_front;
    Vec3 m_right;
    Vec3 m_up;
}; 