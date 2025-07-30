#include "Player.h"
#include "World.h"
#include "BlockManager.h"
#include "Item.h"
#include "Debug.h"
#include <cmath>
#include <algorithm>
#include <iostream> // Added for std::cout
#include <vector> // Added for std::vector

Player::Player() : m_position(0.0f, 64.0f, 0.0f), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f),
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

RaycastResult Player::CastRay(World* world, float maxDistance) const {
    RaycastResult result;
    
    if (!world) {
        return result;
    }
    
    Vec3 rayOrigin = GetCameraPosition();
    Vec3 rayDirection = GetForwardVector();
    
    // Step sizes for DDA algorithm
    Vec3 stepSize;
    stepSize.x = (rayDirection.x == 0.0f) ? 1e30f : abs(1.0f / rayDirection.x);
    stepSize.y = (rayDirection.y == 0.0f) ? 1e30f : abs(1.0f / rayDirection.y);
    stepSize.z = (rayDirection.z == 0.0f) ? 1e30f : abs(1.0f / rayDirection.z);
    
    // Calculate which direction to step in for each axis
    Vec3 step;
    Vec3 sideDist;
    
    int mapX = (int)floor(rayOrigin.x);
    int mapY = (int)floor(rayOrigin.y);
    int mapZ = (int)floor(rayOrigin.z);
    
    // Calculate step and initial sideDist
    if (rayDirection.x < 0) {
        step.x = -1;
        sideDist.x = (rayOrigin.x - mapX) * stepSize.x;
    } else {
        step.x = 1;
        sideDist.x = (mapX + 1.0f - rayOrigin.x) * stepSize.x;
    }
    
    if (rayDirection.y < 0) {
        step.y = -1;
        sideDist.y = (rayOrigin.y - mapY) * stepSize.y;
    } else {
        step.y = 1;
        sideDist.y = (mapY + 1.0f - rayOrigin.y) * stepSize.y;
    }
    
    if (rayDirection.z < 0) {
        step.z = -1;
        sideDist.z = (rayOrigin.z - mapZ) * stepSize.z;
    } else {
        step.z = 1;
        sideDist.z = (mapZ + 1.0f - rayOrigin.z) * stepSize.z;
    }
    
    // Perform DDA
    int side = 0; // 0=X-side, 1=Y-side, 2=Z-side
    float perpWallDist = 0.0f;
    
    while (true) {
        // Jump to next map square, either in x-direction, y-direction, or z-direction
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            sideDist.x += stepSize.x;
            mapX += (int)step.x;
            side = 0;
            perpWallDist = (mapX - rayOrigin.x + (1 - step.x) / 2) / rayDirection.x;
        } else if (sideDist.y < sideDist.z) {
            sideDist.y += stepSize.y;
            mapY += (int)step.y;
            side = 1;
            perpWallDist = (mapY - rayOrigin.y + (1 - step.y) / 2) / rayDirection.y;
        } else {
            sideDist.z += stepSize.z;
            mapZ += (int)step.z;
            side = 2;
            perpWallDist = (mapZ - rayOrigin.z + (1 - step.z) / 2) / rayDirection.z;
        }
        
        // Check if we've exceeded maximum distance
        if (perpWallDist > maxDistance) {
            break;
        }
        
        // Check if we hit a solid block
        Block block = world->GetBlock(mapX, mapY, mapZ);
        if (block.IsSolid()) {
            result.hit = true;
            result.blockPos = Vec3((float)mapX, (float)mapY, (float)mapZ);
            result.distance = perpWallDist;
            result.hitPos = rayOrigin + rayDirection * perpWallDist;
            
            // Calculate face normal based on which side was hit
            switch (side) {
                case 0: // X-side
                    result.normal = Vec3(-step.x, 0.0f, 0.0f);
                    break;
                case 1: // Y-side
                    result.normal = Vec3(0.0f, -step.y, 0.0f);
                    break;
                case 2: // Z-side
                    result.normal = Vec3(0.0f, 0.0f, -step.z);
                    break;
            }
            
            break;
        }
    }
    
    return result;
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

void Player::Update(float deltaTime, World* world, const BlockManager* blockManager) {
    if (!m_isSurvivalMode || !world) return;
    
    ApplyGravity(deltaTime);
    
    // Apply vertical movement from gravity
    if (std::abs(m_verticalVelocity) > 0.001f) {
        Vec3 gravityPosition = Vec3(m_position.x, m_position.y + m_verticalVelocity * deltaTime, m_position.z);
        
        // Handle gravity collision
        Vec3 result = HandleCollision(gravityPosition, world, blockManager);
        
        // If we hit the ground while falling, stop gravity
        if (m_verticalVelocity < 0 && result.y >= m_position.y) {
            // We hit the ground
            m_verticalVelocity = 0.0f;
            m_isOnGround = true;
        } else {
            m_isOnGround = false;
        }
        
        m_position.y = result.y;
    } else {
        // Check if we're still on ground even without vertical velocity
        m_isOnGround = IsOnGround(world, blockManager);
    }
}

bool Player::CheckCollision(const Vec3& newPosition, World* world, const BlockManager* blockManager) const {
    if (!world) return false;
    
    float playerWidth = GetPlayerWidth();
    float halfWidth = playerWidth / 2.0f;
    
    // Check collision for a 2-block tall player
    // Bottom block (feet/legs): y to y+1
    // Top block (torso/head): y+1 to y+2
    
    for (int blockLevel = 0; blockLevel < 2; ++blockLevel) { // 2 blocks tall
        float yCheck = newPosition.y + blockLevel; // Check at y+0 and y+1
        
        // Check multiple points around the player's circumference for precise collision
        // Use symmetric sampling around the center point
        std::vector<std::pair<float, float>> testPoints = {
            {-halfWidth, -halfWidth}, // Bottom-left corner
            {halfWidth, -halfWidth},  // Bottom-right corner
            {halfWidth, halfWidth},   // Top-right corner
            {-halfWidth, halfWidth},  // Top-left corner
            {0.0f, -halfWidth},       // Bottom center
            {halfWidth, 0.0f},        // Right center
            {0.0f, halfWidth},        // Top center
            {-halfWidth, 0.0f},       // Left center
            {0.0f, 0.0f}              // Center point
        };
        
        for (const auto& offset : testPoints) {
            float testX = newPosition.x + offset.first;
            float testZ = newPosition.z + offset.second;
            
            // Use proper rounding to get the block coordinate
            int blockX = static_cast<int>(std::round(testX));
            int blockY = static_cast<int>(std::floor(yCheck));
            int blockZ = static_cast<int>(std::round(testZ));
            
            // However, we need to check if the test point is actually inside the block
            // A block at (bx, by, bz) occupies the space from (bx-0.5, by, bz-0.5) to (bx+0.5, by+1, bz+0.5)
            if (testX >= blockX - 0.5f && testX <= blockX + 0.5f &&
                testZ >= blockZ - 0.5f && testZ <= blockZ + 0.5f &&
                yCheck >= blockY && yCheck < blockY + 1.0f) {
                
                Block block = world->GetBlock(blockX, blockY, blockZ);
                if (block.IsSolid()) {
                    // Skip collision if this is a ground block and we have BlockManager
                    if (blockManager && blockManager->IsGround(block.GetType())) {
                        continue; // Don't collide with ground blocks
                    }
                    return true; // Collision detected
                }
            }
        }
    }
    
    return false; // No collision
}

bool Player::CheckGroundCollision(const Vec3& position, World* world, const BlockManager* blockManager) const {
    if (!world) return false;
    
    float playerWidth = GetPlayerWidth();
    float halfWidth = playerWidth / 2.0f;
    
    // Check only the bottom block of the player (feet level) with symmetric detection
    std::vector<std::pair<float, float>> testPoints = {
        {-halfWidth, -halfWidth}, // Bottom-left corner
        {halfWidth, -halfWidth},  // Bottom-right corner
        {halfWidth, halfWidth},   // Top-right corner
        {-halfWidth, halfWidth},  // Top-left corner
        {0.0f, 0.0f}              // Center point
    };
    
    for (const auto& offset : testPoints) {
        float testX = position.x + offset.first;
        float testZ = position.z + offset.second;
        
        // Use proper rounding to get the block coordinate
        int blockX = static_cast<int>(std::round(testX));
        int blockY = static_cast<int>(std::floor(position.y));
        int blockZ = static_cast<int>(std::round(testZ));
        
        // Check if the test point is actually inside the block
        if (testX >= blockX - 0.5f && testX <= blockX + 0.5f &&
            testZ >= blockZ - 0.5f && testZ <= blockZ + 0.5f &&
            position.y >= blockY && position.y < blockY + 1.0f) {
            
            Block block = world->GetBlock(blockX, blockY, blockZ);
            if (block.IsSolid()) {
                // Skip collision if this is a ground block and we have BlockManager
                if (blockManager && blockManager->IsGround(block.GetType())) {
                    continue; // Don't treat ground blocks as solid ground for landing
                }
                return true; // Ground collision detected
            }
        }
    }
    
    return false;
}

Vec3 Player::HandleCollision(const Vec3& newPosition, World* world, const BlockManager* blockManager) {
    if (!world) return newPosition;
    
    Vec3 result = m_position; // Start with current position
    
    // Handle horizontal movement (X and Z axes) - test each axis independently
    // Test X movement
    Vec3 testX = Vec3(newPosition.x, m_position.y, m_position.z);
    if (!CheckCollision(testX, world, blockManager)) {
        result.x = newPosition.x; // X movement is safe
    }
    
    // Test Z movement
    Vec3 testZ = Vec3(result.x, m_position.y, newPosition.z);
    if (!CheckCollision(testZ, world, blockManager)) {
        result.z = newPosition.z; // Z movement is safe
    }
    
    // Handle vertical movement (Y axis) - this is for gravity/jumping
    Vec3 testY = Vec3(result.x, newPosition.y, result.z);
    
    // Use the ground-block-skipping collision check - this will ignore ground blocks entirely
    if (!CheckCollision(testY, world, blockManager)) {
        result.y = newPosition.y; // Y movement is safe (no collision with solid blocks)
    } else if (newPosition.y < m_position.y) {
        // We're falling and hit a solid (non-ground) block - land on top
        float groundLevel = FindGroundLevel(Vec3(result.x, newPosition.y, result.z), world, blockManager);
        result.y = groundLevel;
    }
    // If moving up and hit ceiling, just keep current Y position (result.y = m_position.y)
    
    return result;
}

float Player::FindGroundLevel(const Vec3& position, World* world, const BlockManager* blockManager) const {
    if (!world) return position.y;
    
    // Start from current falling position and work upward to find the ground surface
    int startY = static_cast<int>(std::floor(position.y));
    
    // Look for the highest solid block below the player
    for (int y = startY; y >= 0; y--) {
        Vec3 testPos = Vec3(position.x, static_cast<float>(y), position.z);
        if (CheckGroundCollision(testPos, world, blockManager)) {
            // Found solid ground, player should stand on top of this block
            return static_cast<float>(y + 1);
        }
    }
    
    // No ground found, return original position
    return position.y;
}

bool Player::IsOnGround(World* world, const BlockManager* blockManager) const {
    if (!world) return false;
    
    // Check slightly below the player's feet (bottom block only)
    Vec3 testPos = Vec3(m_position.x, m_position.y - 0.01f, m_position.z);
    return CheckGroundCollision(testPos, world, blockManager);
}

void Player::Jump() {
    // Only allow jumping if in survival mode and on ground
    if (m_isSurvivalMode && m_isOnGround) {
        m_verticalVelocity = JUMP_VELOCITY;
        m_isOnGround = false; // Player is no longer on ground after jumping
    }
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

void Player::ProcessInput(GLFWwindow* window, float deltaTime, World* world, const BlockManager* blockManager) {
    float velocity = m_movementSpeed * deltaTime;
    Vec3 intendedPosition = m_position;
    
    // Handle movement input
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (m_isSurvivalMode) {
            // In survival mode, only move horizontally
            Vec3 horizontalFront = Vec3(m_front.x, 0.0f, m_front.z);
            float length = std::sqrt(horizontalFront.x * horizontalFront.x + horizontalFront.z * horizontalFront.z);
            if (length > 0.001f) {
                horizontalFront.x /= length;
                horizontalFront.z /= length;
                intendedPosition = intendedPosition + horizontalFront * velocity;
            }
        } else {
            intendedPosition = intendedPosition + m_front * velocity;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (m_isSurvivalMode) {
            Vec3 horizontalFront = Vec3(m_front.x, 0.0f, m_front.z);
            float length = std::sqrt(horizontalFront.x * horizontalFront.x + horizontalFront.z * horizontalFront.z);
            if (length > 0.001f) {
                horizontalFront.x /= length;
                horizontalFront.z /= length;
                intendedPosition = intendedPosition - horizontalFront * velocity;
            }
        } else {
            intendedPosition = intendedPosition - m_front * velocity;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        intendedPosition = intendedPosition - m_right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        intendedPosition = intendedPosition + m_right * velocity;
    }
    
    // Handle vertical movement based on mode
    if (!m_isSurvivalMode) {
        // Creative mode - free vertical movement
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            intendedPosition.y += velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            intendedPosition.y -= velocity;
    } else {
        // Survival mode - jumping only
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            Jump();
        }
    }
    
    // Apply collision detection
    if (m_isSurvivalMode && world) {
        // For survival mode, apply gravity in Update() method
        // Here we only handle player input movement with collision
        Vec3 movementResult = HandleCollision(intendedPosition, world, blockManager);
        
        // Only update horizontal position from input, gravity handles vertical
        m_position.x = movementResult.x;
        m_position.z = movementResult.z;
        
        // Update ground state
        m_isOnGround = IsOnGround(world, blockManager);
        
        // If we just landed on ground, stop falling
        if (m_isOnGround && m_verticalVelocity < 0) {
            m_verticalVelocity = 0.0f;
        }
    } else {
        // Creative mode - free movement
        m_position = intendedPosition;
    }
}

Mat4 Player::GetViewMatrix() const {
    Mat4 view;
    
    // Get camera position at eye level (1.5 blocks above feet)
    Vec3 cameraPos = GetCameraPosition();
    
    // Create look-at matrix manually
    Vec3 target = cameraPos + m_front;
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
    
    // Calculate camera coordinate system
    Vec3 f = Vec3(target.x - cameraPos.x, target.y - cameraPos.y, target.z - cameraPos.z);
    float f_len = sqrt(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= f_len; f.y /= f_len; f.z /= f_len;
    
    Vec3 s = Vec3(f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x);
    float s_len = sqrt(s.x*s.x + s.y*s.y + s.z*s.z);
    s.x /= s_len; s.y /= s_len; s.z /= s_len;
    
    Vec3 u = Vec3(s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x);
    
    // Build view matrix using camera position instead of player position
    view.m[0] = s.x;
    view.m[4] = s.y;
    view.m[8] = s.z;
    view.m[12] = -(s.x * cameraPos.x + s.y * cameraPos.y + s.z * cameraPos.z);
    
    view.m[1] = u.x;
    view.m[5] = u.y;
    view.m[9] = u.z;
    view.m[13] = -(u.x * cameraPos.x + u.y * cameraPos.y + u.z * cameraPos.z);
    
    view.m[2] = -f.x;
    view.m[6] = -f.y;
    view.m[10] = -f.z;
    view.m[14] = f.x * cameraPos.x + f.y * cameraPos.y + f.z * cameraPos.z;
    
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

Vec3 Player::GetCameraPosition() const {
    // Camera should be at eye level, which is approximately 1.5 blocks above the feet
    // Since m_position represents the center bottom of the player (feet level),
    // we add the eye height offset (1.5f works for proper block indicator alignment)
    return Vec3(m_position.x, m_position.y + 1.2f, m_position.z);
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

void Player::InitializeTestInventory(ItemManager* itemManager) {
    if (itemManager) {
        m_inventory.populateTestHotbar(itemManager);
        DEBUG_INVENTORY("Player inventory initialized with test items");
    } else {
        DEBUG_WARNING("ItemManager is null, could not initialize test inventory");
    }
} 