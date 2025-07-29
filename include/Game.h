#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "World.h"
#include "Player.h"
#include "Server.h"
#include "NetworkClient.h"
#include "ServerDiscovery.h"
#include <memory>
#include <unordered_map>

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
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

private:
    GLFWwindow* m_window;
    Renderer m_renderer;
    GameState m_currentState;
    bool m_shouldClose;
    
    // Minecraft world and player
    std::unique_ptr<World> m_world;
    std::unique_ptr<Player> m_player;
    
    // Networking
    std::unique_ptr<Server> m_server;
    std::unique_ptr<NetworkClient> m_networkClient;
    std::unique_ptr<ServerDiscovery> m_serverDiscovery;
    bool m_isHost;
    std::unordered_map<uint32_t, PlayerPosition> m_otherPlayers;
    
    // Mouse input
    bool m_firstMouse;
    double m_lastX, m_lastY;
    
    // Timing
    float m_deltaTime;
    float m_lastFrame;

    // State management
    void SetState(GameState newState);
    void UpdateMainMenu();
    void UpdateGame();
    void RenderMainMenu();
    void RenderGame();
    
    // Networking
    void StartHost();
    void JoinServer(const std::string& serverIP);
    void SendPlayerPosition();
    void TestUDPConnectivity(const std::string& targetIP);
    void OnPlayerJoin(uint32_t playerId, const PlayerPosition& position);
    void OnPlayerLeave(uint32_t playerId);
    void OnPlayerPositionUpdate(uint32_t playerId, const PlayerPosition& position);

    // Window callbacks
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void ErrorCallback(int error, const char* description);

    // Static instance for callbacks
    static Game* s_instance;
}; 