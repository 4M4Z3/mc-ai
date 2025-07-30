#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "World.h"
#include "Player.h"
#include "Server.h"
#include "NetworkClient.h"
#include "ServerDiscovery.h"
#include "Item.h"
#include <memory>
#include <unordered_map>
#include <chrono>
#include <queue> // Added for thread-safe queue
#include <mutex> // Added for mutex
#include <vector> // Added for chunk data storage
#include <set> // Added for set of broken blocks
#include <tuple> // Added for tuple of coordinates

// Forward declare ImFont to avoid including the full ImGui header
struct ImFont;

enum class GameState {
    MAIN_MENU,
    GAME
};

// Structure for smooth player interpolation
struct InterpolatedPlayer {
    PlayerPosition currentPos;
    PlayerPosition previousPos;
    std::chrono::steady_clock::time_point lastUpdateTime;
    std::chrono::steady_clock::time_point previousUpdateTime;
    
    // Get interpolated position based on current time
    PlayerPosition GetInterpolatedPosition() const {
        auto now = std::chrono::steady_clock::now();
        
        // Calculate time since last update
        auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();
        
        // Don't interpolate if too much time has passed (player might be disconnected/lagging)
        if (timeSinceUpdate > 1000) { // 1 second timeout
            return currentPos;
        }
        
        // Calculate interpolation time window
        auto updateInterval = std::chrono::duration_cast<std::chrono::milliseconds>(lastUpdateTime - previousUpdateTime).count();
        
        if (updateInterval <= 0) {
            return currentPos;
        }
        
        // Calculate interpolation factor (0.0 = previous, 1.0 = current, >1.0 = extrapolate)
        float t = static_cast<float>(timeSinceUpdate) / static_cast<float>(updateInterval);
        
        // Clamp to reasonable bounds for interpolation/light extrapolation
        t = std::max(0.0f, std::min(t, 1.2f));
        
        // Linear interpolation
        PlayerPosition interpolated;
        interpolated.x = previousPos.x + t * (currentPos.x - previousPos.x);
        interpolated.y = previousPos.y + t * (currentPos.y - previousPos.y);
        interpolated.z = previousPos.z + t * (currentPos.z - previousPos.z);
        interpolated.yaw = previousPos.yaw + t * (currentPos.yaw - previousPos.yaw);
        interpolated.pitch = previousPos.pitch + t * (currentPos.pitch - previousPos.pitch);
        interpolated.playerId = currentPos.playerId;
        
        return interpolated;
    }
    
    // Update with new position data
    void UpdatePosition(const PlayerPosition& newPos) {
        previousPos = currentPos;
        previousUpdateTime = lastUpdateTime;
        currentPos = newPos;
        lastUpdateTime = std::chrono::steady_clock::now();
    }
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
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

private:
    GLFWwindow* m_window;
    Renderer m_renderer;
    GameState m_currentState;
    bool m_shouldClose;
    
    // Minecraft world and player
    std::unique_ptr<World> m_world;
    std::unique_ptr<Player> m_player;
    
    // Item system
    std::unique_ptr<ItemManager> m_itemManager;
    
    // Block targeting
    RaycastResult m_targetBlock;
    
    // Thread-safe queue for block breaks received from network
    struct PendingBlockBreak {
        uint32_t playerId;
        int32_t x, y, z;
    };
    std::queue<PendingBlockBreak> m_pendingBlockBreaks;
    std::mutex m_pendingBlockBreaksMutex;
    
    // Thread-safe queue for block updates received from network
    struct PendingBlockUpdate {
        uint32_t playerId;
        int32_t x, y, z;
        uint16_t blockType;
    };
    std::queue<PendingBlockUpdate> m_pendingBlockUpdates;
    std::mutex m_pendingBlockUpdatesMutex;
    
    // Thread-safe queue for chunk data received from network
    struct PendingChunkData {
        int32_t chunkX, chunkZ;
        std::vector<uint16_t> blockData; // Copy the block data
    };
    std::queue<PendingChunkData> m_pendingChunkData;
    std::mutex m_pendingChunkDataMutex;
    
    // Networking
    std::unique_ptr<Server> m_server;
    std::unique_ptr<NetworkClient> m_networkClient;
    std::unique_ptr<ServerDiscovery> m_serverDiscovery;
    bool m_isHost;
    std::unordered_map<uint32_t, InterpolatedPlayer> m_otherPlayers;
    uint32_t m_myPlayerId; // Store our own player ID to avoid self-rendering
    
    // World seed synchronization
    int32_t m_worldSeed;
    bool m_worldSeedReceived;
    
    // Game time synchronization
    float m_gameTime; // Current game time (0-900 seconds)
    bool m_gameTimeReceived;
    
    // Player position change detection for networking
    PlayerPosition m_lastSentPlayerPosition;
    bool m_hasLastSentPosition;
    static constexpr float POSITION_CHANGE_THRESHOLD = 0.05f; // Minimum position change to send update
    static constexpr float ROTATION_CHANGE_THRESHOLD = 1.0f;  // Minimum rotation change (degrees) to send update
    
    // Mouse input
    bool m_firstMouse;
    double m_lastX, m_lastY;
    
    // Timing
    float m_deltaTime;
    float m_lastFrame;
    
    // UI Fonts
    ImFont* m_fontSmall;
    ImFont* m_fontDefault;
    ImFont* m_fontLarge;
    ImFont* m_fontTitle;
    
    // Pause menu
    bool m_showPauseMenu;
    
    // Inventory menu
    bool m_showInventory;
    
    // Hotbar selection
    int m_selectedHotbarSlot;

    // State management
    void SetState(GameState newState);
    void UpdateMainMenu();
    void UpdateGame();
    void RenderMainMenu();
    void RenderGame();
    void RenderPauseMenu();
    void RenderInventory();
    void RenderCustomInventorySlot(const InventorySlot& slot, float x, float y, float size, int slotIndex);
    void RenderCustomHotbarSlot(const InventorySlot& slot, float x, float y, float size, int slotIndex, bool isSelected);
    void RenderHotbar();
    
    // Helper method to get current interpolated positions for rendering
    std::unordered_map<uint32_t, PlayerPosition> GetInterpolatedPlayerPositions() const;
    
    // Networking
    void StartHost();
    void JoinServer(const std::string& serverIP);
    void SendPlayerPosition();
    void TestUDPConnectivity(const std::string& targetIP);
    void OnPlayerJoin(uint32_t playerId, const PlayerPosition& position);
    void OnPlayerLeave(uint32_t playerId);
    void OnPlayerPositionUpdate(uint32_t playerId, const PlayerPosition& position);
    void OnWorldSeedReceived(int32_t worldSeed);
    void OnGameTimeReceived(float gameTime);
    void OnMyPlayerIdReceived(uint32_t myPlayerId); // Handle receiving own player ID
    void OnBlockBreakReceived(uint32_t playerId, int32_t x, int32_t y, int32_t z);
    void OnBlockUpdateReceived(uint32_t playerId, int32_t x, int32_t y, int32_t z, uint16_t blockType);
    void OnChunkDataReceived(int32_t chunkX, int32_t chunkZ, const uint16_t* blockData);
    
    // Time utility methods
    bool IsDay() const;
    bool IsNight() const;
    float GetTimeOfDay() const; // Returns 0.0-1.0 where 0.5 is sunset

    // Window callbacks
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowCloseCallback(GLFWwindow* window);
    static void SignalHandler(int signal);
    static void ErrorCallback(int error, const char* description);

    // Static instance for callbacks
    static Game* s_instance;
}; 