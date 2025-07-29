#include "Player.h"
#include "World.h"
#include <cmath>
#include <algorithm>
#include <iostream> // Added for std::cout

Player::Player() : m_position(0.0f, 5.0f, 3.0f), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f),
                   m_isSurvivalMode(false), m_verticalVelocity(0.0f), m_isOnGround(false) {
    UpdateVectors();
}

Player::Player(float x, float y, float z) : m_position(x, y, z), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f),
                                            m_isSurvivalMode(false), m_verticalVelocity(0.0f), m_isOnGround(false) {
    UpdateVectors();
}

void Player::SetRotation(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = pitch;
    
    // Constrain pitch to avoid flipping
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;
    
    UpdateVectors();
}

void Player::MoveForward(float distance) {
    m_position = m_position + m_front * distance;
}

void Player::MoveBackward(float distance) {
    m_position = m_position - m_front * distance;
}

void Player::MoveLeft(float distance) {
    m_position = m_position - m_right * distance;
}

void Player::MoveRight(float distance) {
    m_position = m_position + m_right * distance;
}

void Player::MoveUp(float distance) {
    m_position.y += distance;
}

void Player::MoveDown(float distance) {
    m_position.y -= distance;
}

bool Player::CanEnterSurvivalMode() const {
    // This method will be called with world context from Game class
    return true; // Placeholder - will be checked in ToggleSurvivalMode with world
}

bool Player::CanEnterSurvivalMode(World* world) const {
    if (!world) return false;
    
    // Check if player would be inside blocks when switching to survival mode
    // Player is 2 blocks tall, so check from feet to head
    return !CheckCollision(m_position, world);
}

void Player::ToggleSurvivalMode() {
    ToggleSurvivalMode(nullptr); // Default version - will be overridden
}

void Player::ToggleSurvivalMode(World* world) {
    if (!m_isSurvivalMode && world && !CanEnterSurvivalMode(world)) {
        std::cout << "Cannot enter survival mode - player is inside blocks!" << std::endl;
        return; // Can't enter survival mode if inside blocks
    }
    
    m_isSurvivalMode = !m_isSurvivalMode;
    
    if (m_isSurvivalMode) {
        // Reset physics when entering survival mode
        m_verticalVelocity = 0.0f;
        m_isOnGround = false;
        std::cout << "Survival mode activated - gravity and collisions enabled" << std::endl;
    } else {
        std::cout << "Creative mode activated - free flight enabled" << std::endl;
    }
}

void Player::ApplyGravity(float deltaTime) {
    if (!m_isSurvivalMode) return;
    
    // Apply gravity acceleration
    m_verticalVelocity -= GRAVITY * deltaTime;
    
    // Clamp to terminal velocity
    if (m_verticalVelocity < -TERMINAL_VELOCITY) {
        m_verticalVelocity = -TERMINAL_VELOCITY;
    }
}

void Player::Update(float deltaTime) {
    if (!m_isSurvivalMode) return;
    
    ApplyGravity(deltaTime);
    
    // Apply vertical movement from gravity
    if (std::abs(m_verticalVelocity) > 0.001f) {
        Vec3 newPosition = Vec3(m_position.x, m_position.y + m_verticalVelocity * deltaTime, m_position.z);
        
        // For now, we'll handle ground collision in ProcessInput where we have access to world
        m_position.y = newPosition.y;
    }
}

bool Player::CheckCollision(const Vec3& newPosition, World* world) const {
    if (!world) return false;
    
    float playerWidth = GetPlayerWidth();
    float playerHeight = GetPlayerHeight();
    
    // Check collision at player's feet, middle, and head
    for (float yOffset = 0.0f; yOffset < playerHeight; yOffset += 0.5f) {
        for (float xOffset = -playerWidth/2; xOffset <= playerWidth/2; xOffset += playerWidth/2) {
            for (float zOffset = -playerWidth/2; zOffset <= playerWidth/2; zOffset += playerWidth/2) {
                int blockX = static_cast<int>(std::floor(newPosition.x + xOffset));
                int blockY = static_cast<int>(std::floor(newPosition.y + yOffset));
                int blockZ = static_cast<int>(std::floor(newPosition.z + zOffset));
                
                Block block = world->GetBlock(blockX, blockY, blockZ);
                if (block.IsSolid()) {
                    return true; // Collision detected
                }
            }
        }
    }
    
    return false; // No collision
}

Vec3 Player::HandleCollision(const Vec3& newPosition, World* world) {
    if (!world) return newPosition;
    
    Vec3 result = newPosition;
    
    // Check X-axis collision
    Vec3 testX = Vec3(newPosition.x, m_position.y, m_position.z);
    if (CheckCollision(testX, world)) {
        result.x = m_position.x; // Revert X movement
    }
    
    // Check Z-axis collision
    Vec3 testZ = Vec3(result.x, m_position.y, newPosition.z);
    if (CheckCollision(testZ, world)) {
        result.z = m_position.z; // Revert Z movement
    }
    
    // Check Y-axis collision (gravity/vertical movement)
    Vec3 testY = Vec3(result.x, newPosition.y, result.z);
    if (CheckCollision(testY, world)) {
        if (newPosition.y < m_position.y) {
            // Falling down and hit ground
            result.y = std::ceil(m_position.y); // Snap to block surface
            // Note: Ground collision handling will be done in Update() method
        } else {
            // Moving up and hit ceiling
            result.y = m_position.y; // Revert Y movement
        }
    }
    
    return result;
}

bool Player::IsOnGround(World* world) const {
    if (!world) return false;
    
    // Check slightly below the player's feet
    Vec3 testPos = Vec3(m_position.x, m_position.y - 0.1f, m_position.z);
    return CheckCollision(testPos, world);
}

void Player::ProcessMouseMovement(float xOffset, float yOffset, float sensitivity) {
    xOffset *= sensitivity;
    yOffset *= sensitivity;
    
    m_yaw += xOffset;
    m_pitch += yOffset;
    
    // Constrain pitch
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;
    
    UpdateVectors();
}

void Player::ProcessInput(GLFWwindow* window, float deltaTime, World* world) {
    float velocity = m_movementSpeed * deltaTime;
    Vec3 newPosition = m_position;
    
    // Handle movement input
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (m_isSurvivalMode) {
            // In survival mode, only move horizontally
            Vec3 horizontalFront = Vec3(m_front.x, 0.0f, m_front.z);
            float length = std::sqrt(horizontalFront.x * horizontalFront.x + horizontalFront.z * horizontalFront.z);
            if (length > 0.001f) {
                horizontalFront.x /= length;
                horizontalFront.z /= length;
                newPosition = newPosition + horizontalFront * velocity;
            }
        } else {
            newPosition = newPosition + m_front * velocity;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (m_isSurvivalMode) {
            Vec3 horizontalFront = Vec3(m_front.x, 0.0f, m_front.z);
            float length = std::sqrt(horizontalFront.x * horizontalFront.x + horizontalFront.z * horizontalFront.z);
            if (length > 0.001f) {
                horizontalFront.x /= length;
                horizontalFront.z /= length;
                newPosition = newPosition - horizontalFront * velocity;
            }
        } else {
            newPosition = newPosition - m_front * velocity;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        newPosition = newPosition - m_right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        newPosition = newPosition + m_right * velocity;
    }
    
    // Vertical movement only in creative mode
    if (!m_isSurvivalMode) {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            newPosition.y += velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            newPosition.y -= velocity;
    }
    
    // Apply collision detection in survival mode
    if (m_isSurvivalMode && world) {
        // Handle horizontal movement collision
        Vec3 horizontalPosition = HandleCollision(newPosition, world);
        m_position.x = horizontalPosition.x;
        m_position.z = horizontalPosition.z;
        
        // Handle vertical collision (ground detection)
        Vec3 testGroundPos = Vec3(m_position.x, m_position.y, m_position.z);
        if (CheckCollision(testGroundPos, world)) {
            // We're intersecting with ground, snap to surface and stop falling
            m_position.y = std::floor(m_position.y) + 1.0f; // Snap to top of block
            m_verticalVelocity = 0.0f;
            m_isOnGround = true;
        } else {
            m_isOnGround = false;
        }
    } else {
        m_position = newPosition;
    }
}

Mat4 Player::GetViewMatrix() const {
    Mat4 view;
    
    // Create look-at matrix manually
    Vec3 target = m_position + m_front;
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
    
    // Calculate camera coordinate system
    Vec3 f = Vec3(target.x - m_position.x, target.y - m_position.y, target.z - m_position.z);
    float f_len = sqrt(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= f_len; f.y /= f_len; f.z /= f_len;
    
    Vec3 s = Vec3(f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x);
    float s_len = sqrt(s.x*s.x + s.y*s.y + s.z*s.z);
    s.x /= s_len; s.y /= s_len; s.z /= s_len;
    
    Vec3 u = Vec3(s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x);
    
    // Build view matrix
    view.m[0] = s.x;
    view.m[4] = s.y;
    view.m[8] = s.z;
    view.m[12] = -(s.x * m_position.x + s.y * m_position.y + s.z * m_position.z);
    
    view.m[1] = u.x;
    view.m[5] = u.y;
    view.m[9] = u.z;
    view.m[13] = -(u.x * m_position.x + u.y * m_position.y + u.z * m_position.z);
    
    view.m[2] = -f.x;
    view.m[6] = -f.y;
    view.m[10] = -f.z;
    view.m[14] = f.x * m_position.x + f.y * m_position.y + f.z * m_position.z;
    
    view.m[3] = 0.0f;
    view.m[7] = 0.0f;
    view.m[11] = 0.0f;
    view.m[15] = 1.0f;
    
    return view;
}

Vec3 Player::GetForwardVector() const {
    return m_front;
}

Vec3 Player::GetRightVector() const {
    return m_right;
}

Vec3 Player::GetUpVector() const {
    return m_up;
}

void Player::UpdateVectors() {
    // Calculate the new front vector
    Vec3 front;
    front.x = cos(m_yaw * M_PI / 180.0f) * cos(m_pitch * M_PI / 180.0f);
    front.y = sin(m_pitch * M_PI / 180.0f);
    front.z = sin(m_yaw * M_PI / 180.0f) * cos(m_pitch * M_PI / 180.0f);
    
    // Normalize front vector
    float length = sqrt(front.x*front.x + front.y*front.y + front.z*front.z);
    m_front.x = front.x / length;
    m_front.y = front.y / length;
    m_front.z = front.z / length;
    
    // Calculate right and up vectors
    Vec3 worldUp = Vec3(0.0f, 1.0f, 0.0f);
    
    // Right = cross(front, worldUp)
    m_right.x = m_front.y * worldUp.z - m_front.z * worldUp.y;
    m_right.y = m_front.z * worldUp.x - m_front.x * worldUp.z;
    m_right.z = m_front.x * worldUp.y - m_front.y * worldUp.x;
    
    // Normalize right vector
    length = sqrt(m_right.x*m_right.x + m_right.y*m_right.y + m_right.z*m_right.z);
    m_right.x /= length;
    m_right.y /= length;
    m_right.z /= length;
    
    // Up = cross(right, front)
    m_up.x = m_right.y * m_front.z - m_right.z * m_front.y;
    m_up.y = m_right.z * m_front.x - m_right.x * m_front.z;
    m_up.z = m_right.x * m_front.y - m_right.y * m_front.x;
    
    // Normalize up vector
    length = sqrt(m_up.x*m_up.x + m_up.y*m_up.y + m_up.z*m_up.z);
    m_up.x /= length;
    m_up.y /= length;
    m_up.z /= length;
} 