#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <atomic>
#include <memory>
#include "World.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

struct PlayerPosition {
    float x, y, z;
    float yaw, pitch;
    uint32_t playerId;
};

// Base message header
struct NetworkMessageHeader {
    enum Type {
        PLAYER_JOIN = 1,
        PLAYER_LEAVE = 2,
        PLAYER_POSITION = 3,
        PLAYER_LIST = 4,
        WORLD_SEED = 5,
        TIME_SYNC = 6,
        BLOCK_BREAK = 7,
        CHUNK_REQUEST = 8,
        CHUNK_DATA = 9,
        MY_PLAYER_ID = 10,
        BLOCK_UPDATE = 11
    };
    
    uint8_t type;
    uint32_t playerId;
};

// Small message for most communications
struct NetworkMessage {
    NetworkMessageHeader header;
    PlayerPosition position;
    int32_t worldSeed;
    float gameTime;
    
    // Block data
    struct {
        int32_t x, y, z;
        uint8_t blockType; // For block updates, 0 for breaks
    } blockData;
    
    // Chunk request data
    struct {
        int32_t chunkX, chunkZ;
    } chunkRequest;
};

// Large message for chunk data only
struct ChunkDataMessage {
    NetworkMessageHeader header;
    int32_t chunkX, chunkZ;
    uint8_t blocks[16 * 256 * 16];
};

// Server announcement for UDP broadcast discovery
struct ServerAnnouncement {
    char magic[8] = {'M', 'C', '_', 'S', 'E', 'R', 'V', 'R'}; // Magic bytes to identify our packets
    char serverName[64];
    char serverIP[16];
    uint16_t serverPort;
    uint16_t playerCount;
    uint16_t maxPlayers;
    uint32_t timestamp; // For freshness checking
};

class Server {
public:
    Server();
    ~Server();
    
    bool Start(int port = 8080);
    void Stop();
    bool IsRunning() const { return m_running; }
    
    // Get connected player count
    int GetPlayerCount();
    
    // Get server info
    std::string GetServerInfo();
    
    // Get local network IP address
    std::string GetLocalIPAddress();

    // World seed management
    int32_t GetWorldSeed() const { return m_worldSeed; }
    
    // Time management
    float GetGameTime() const { return m_gameTime; }
    bool IsDay() const;
    bool IsNight() const;
    
    // Chunk management
    void SendChunkData(socket_t clientSocket, int32_t chunkX, int32_t chunkZ);
    void HandleChunkRequest(socket_t clientSocket, int32_t chunkX, int32_t chunkZ);

private:
    void AcceptClients();
    void HandleClient(socket_t clientSocket, uint32_t playerId);
    void BroadcastToAllClients(const NetworkMessage& message, uint32_t excludePlayerId = 0);
    void SendPlayerList(socket_t clientSocket);
    void SendWorldSeed(socket_t clientSocket); // Send world seed to connecting client
    void SendGameTime(socket_t clientSocket); // Send current game time to connecting client
    void SendMyPlayerId(socket_t clientSocket, uint32_t playerId); // Send client their own player ID
    void BroadcastGameTime(); // Broadcast time sync to all clients
    
    // Calculate proper spawn position
    PlayerPosition CalculateSpawnPosition(uint32_t playerId);
    
    // Time management
    void UpdateGameTime();
    void StartTimeSync();
    void StopTimeSync();

    // UDP Broadcast for server discovery
    void StartBroadcast();
    void StopBroadcast();
    void BroadcastServerPresence();
    std::string GetBroadcastAddress(const std::string& localIP);
    
    bool InitializeWinsock();
    void CleanupWinsock();
    
    socket_t m_serverSocket;
    std::atomic<bool> m_running;
    std::thread m_acceptThread;
    
    // UDP Broadcast components
    socket_t m_broadcastSocket;
    std::atomic<bool> m_broadcasting;
    std::thread m_broadcastThread;
    
    // Client management
    struct ClientInfo {
        socket_t socket;
        uint32_t playerId;
        std::thread thread;
        PlayerPosition position;
        bool active;
    };
    
    std::vector<std::unique_ptr<ClientInfo>> m_clients;
    std::mutex m_clientsMutex;
    
    uint32_t m_nextPlayerId;
    int m_port;
    int32_t m_worldSeed; // Server-managed world seed
    std::unique_ptr<World> m_world; // Server-side world for spawn calculations
    
    // Time management
    float m_gameTime; // Current game time in seconds (0-900 for 15 minute cycle)
    std::chrono::steady_clock::time_point m_gameStartTime;
    std::chrono::steady_clock::time_point m_lastTimeSyncBroadcast;
    std::thread m_timeUpdateThread;
    std::atomic<bool> m_timeUpdating;
    
#ifdef _WIN32
    bool m_winsockInitialized;
#endif
}; 