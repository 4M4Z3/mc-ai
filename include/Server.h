#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <atomic>

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

struct NetworkMessage {
    enum Type {
        PLAYER_JOIN = 1,
        PLAYER_LEAVE = 2,
        PLAYER_POSITION = 3,
        PLAYER_LIST = 4
    };
    
    uint8_t type;
    uint32_t playerId;
    PlayerPosition position;
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

private:
    void AcceptClients();
    void HandleClient(socket_t clientSocket, uint32_t playerId);
    void BroadcastToAllClients(const NetworkMessage& message, uint32_t excludePlayerId = 0);
    void SendPlayerList(socket_t clientSocket);
    
    bool InitializeWinsock();
    void CleanupWinsock();
    
    socket_t m_serverSocket;
    std::atomic<bool> m_running;
    std::thread m_acceptThread;
    
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
    
#ifdef _WIN32
    bool m_winsockInitialized;
#endif
}; 