#pragma once

#include "Server.h" // For PlayerPosition and NetworkMessage structs
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <functional>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    
    // Connection management
    bool Connect(const std::string& serverIP, int port = 8080);
    void Disconnect();
    bool IsConnected() const { return m_connected; }
    
    // Send player position to server
    void SendPlayerPosition(const PlayerPosition& position);
    
    // Set callback for receiving other players' positions
    void SetPlayerJoinCallback(std::function<void(uint32_t playerId, const PlayerPosition&)> callback);
    void SetPlayerLeaveCallback(std::function<void(uint32_t playerId)> callback);
    void SetPlayerPositionCallback(std::function<void(uint32_t playerId, const PlayerPosition&)> callback);
    void SetWorldSeedCallback(std::function<void(int32_t worldSeed)> callback);
    
    // Get other players
    std::unordered_map<uint32_t, PlayerPosition> GetOtherPlayers() const;
    
    // Get connection info
    std::string GetConnectionInfo() const;

private:
    void ReceiveMessages();
    void ProcessMessage(const NetworkMessage& message);
    
    bool InitializeWinsock();
    void CleanupWinsock();
    
    socket_t m_socket;
    std::atomic<bool> m_connected;
    std::thread m_receiveThread;
    
    // Other players tracking
    std::unordered_map<uint32_t, PlayerPosition> m_otherPlayers;
    mutable std::mutex m_otherPlayersMutex;
    
    // Callbacks
    std::function<void(uint32_t, const PlayerPosition&)> m_onPlayerJoin;
    std::function<void(uint32_t)> m_onPlayerLeave;
    std::function<void(uint32_t, const PlayerPosition&)> m_onPlayerPosition;
    std::function<void(int32_t)> m_onWorldSeed;
    
    std::string m_serverIP;
    int m_serverPort;
    
#ifdef _WIN32
    bool m_winsockInitialized;
#endif
}; 