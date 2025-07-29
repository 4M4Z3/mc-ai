#include "Player.h"
#include <cmath>

Player::Player() : m_position(0.0f, 5.0f, 3.0f), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f) {
    UpdateVectors();
}

Player::Player(float x, float y, float z) : m_position(x, y, z), m_yaw(-90.0f), m_pitch(0.0f), m_movementSpeed(5.0f) {
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

void Player::ProcessInput(GLFWwindow* window, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        MoveForward(velocity);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        MoveBackward(velocity);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        MoveLeft(velocity);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        MoveRight(velocity);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        MoveUp(velocity);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        MoveDown(velocity);
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