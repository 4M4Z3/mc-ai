#pragma once

#include <cmath>

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <epoxy/gl.h>
#endif
#include <GLFW/glfw3.h>

// Forward declarations
class World;
class BlockManager;

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
    
    float Length() const {
        return sqrt(x*x + y*y + z*z);
    }
    
    Vec3 Normalize() const {
        float len = Length();
        if (len > 0.0f) {
            return Vec3(x/len, y/len, z/len);
        }
        return Vec3(0.0f, 0.0f, 0.0f);
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

// Structure to hold ray casting results
struct RaycastResult {
    bool hit;               // Whether the ray hit a block
    Vec3 blockPos;         // World position of the hit block (as integers)
    Vec3 hitPos;           // Exact hit position on the block
    Vec3 normal;           // Face normal of the hit surface
    float distance;        // Distance from ray origin to hit point
    
    RaycastResult() : hit(false), blockPos(0, 0, 0), hitPos(0, 0, 0), normal(0, 0, 0), distance(0.0f) {}
};

class Player {
public:
    Player();
    Player(float x, float y, float z);
    
    // Position and rotation
    Vec3 GetPosition() const { return m_position; }
    Vec3 GetCameraPosition() const; // Eye level position for camera
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
    
    // Ray casting for block targeting
    RaycastResult CastRay(World* world, float maxDistance = 5.0f) const;
    
    // Survival mode
    bool IsSurvivalMode() const { return m_isSurvivalMode; }
    void SetSurvivalMode(bool enabled) { m_isSurvivalMode = enabled; }
    bool CanEnterSurvivalMode() const;
    bool CanEnterSurvivalMode(class World* world) const;
    void ToggleSurvivalMode();
    void ToggleSurvivalMode(class World* world);
    
    // Physics for survival mode
    void ApplyGravity(float deltaTime);
    void Update(float deltaTime, class World* world, const BlockManager* blockManager = nullptr);
    
    // Collision detection
    bool CheckCollision(const Vec3& newPosition, class World* world, const BlockManager* blockManager = nullptr) const;
    bool CheckGroundCollision(const Vec3& position, class World* world, const BlockManager* blockManager = nullptr) const;
    Vec3 HandleCollision(const Vec3& newPosition, class World* world, const BlockManager* blockManager = nullptr);
    float FindGroundLevel(const Vec3& position, class World* world, const BlockManager* blockManager = nullptr) const;
    
    // Player dimensions
    float GetPlayerHeight() const { return 2.0f; }  // 2 blocks tall
    float GetPlayerWidth() const { return 0.6f; }   // 0.6 blocks wide
    
    // Mouse look
    void ProcessMouseMovement(float xOffset, float yOffset, float sensitivity = 0.1f);
    
    // Input processing
    void ProcessInput(GLFWwindow* window, float deltaTime, class World* world, const BlockManager* blockManager = nullptr);
    
    // Camera matrices
    Mat4 GetViewMatrix() const;
    Vec3 GetForwardVector() const;
    Vec3 GetRightVector() const;
    Vec3 GetUpVector() const;
    
    // Ground collision
    bool IsOnGround(class World* world, const BlockManager* blockManager = nullptr) const;
    
    // Jumping
    void Jump();

private:
    Vec3 m_position;  // Player position at CENTER of feet level (ground level)
    float m_yaw;    // Rotation around Y axis
    float m_pitch;  // Rotation around X axis
    
    // Movement speed
    float m_movementSpeed;
    
    // Survival mode physics
    bool m_isSurvivalMode;
    float m_verticalVelocity;  // For gravity
    bool m_isOnGround;
    
    // Physics constants - Minecraft standard values
    static constexpr float GRAVITY = 32.0f;        // blocks per second squared (Minecraft standard)
    static constexpr float TERMINAL_VELOCITY = 78.4f; // Maximum fall speed
    static constexpr float JUMP_VELOCITY = 8.94f;  // Initial upward velocity when jumping (calculated for 1.25 block height)
    
    // Player positioning:
    // - m_position represents the CENTER of the player at GROUND LEVEL (feet)
    // - Camera is at m_position.y + 1.5 (eye level - aligns with head position in model)
    // - Total player height is 1.8 blocks
    // - Collision detection uses m_position as center for entire 1.8 block height
    // - Player spans from m_position.y to m_position.y + 1.8
    
    // Helper functions
    void UpdateVectors();
    Vec3 m_front;
    Vec3 m_right;
    Vec3 m_up;
}; 