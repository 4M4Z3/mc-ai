#pragma once

#include <GLFW/glfw3.h>
#include "Renderer.h"

enum class GameState {
    MAIN_MENU,
    GAME
};

class Game {
public:
    Game();
    ~Game();

    bool Initialize(int windowWidth = 1280, int windowHeight = 720);
    void Run();
    void Shutdown();

    // Input handling
    void ProcessInput();
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    GLFWwindow* m_window;
    Renderer m_renderer;
    GameState m_currentState;
    bool m_shouldClose;

    // State management
    void SetState(GameState newState);
    void UpdateMainMenu();
    void UpdateGame();
    void RenderMainMenu();
    void RenderGame();

    // Window callbacks
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void ErrorCallback(int error, const char* description);

    // Static instance for callbacks
    static Game* s_instance;
}; 