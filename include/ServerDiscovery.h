#pragma once

#include "Server.h" // For ServerAnnouncement struct
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <atomic>
#include <chrono>

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

struct DiscoveredServer {
    std::string name;
    std::string ip;
    uint16_t port;
    uint16_t playerCount;
    uint16_t maxPlayers;
    std::chrono::steady_clock::time_point lastSeen;
    
    // Helper method to check if server is still active (within timeout)
    bool IsActive(int timeoutSeconds = 10) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSeen);
        return elapsed.count() < timeoutSeconds;
    }
    
    std::string GetDisplayName() const {
        return name + " (" + ip + ":" + std::to_string(port) + ") - " + 
               std::to_string(playerCount) + "/" + std::to_string(maxPlayers) + " players";
    }
};

class ServerDiscovery {
public:
    ServerDiscovery();
    ~ServerDiscovery();
    
    bool Start();
    void Stop();
    bool IsRunning() const { return m_running; }
    
    // Get list of discovered servers (only active ones)
    std::vector<DiscoveredServer> GetDiscoveredServers();
    
    // Clear old/inactive servers
    void CleanupOldServers();

private:
    void ListenForBroadcasts();
    void ProcessServerAnnouncement(const ServerAnnouncement& announcement, const std::string& fromIP);
    
    bool InitializeWinsock();
    void CleanupWinsock();
    
    socket_t m_listenSocket;
    std::atomic<bool> m_running;
    std::thread m_listenThread;
    
    // Discovered servers storage
    std::unordered_map<std::string, DiscoveredServer> m_discoveredServers; // Key: IP:PORT
    std::mutex m_serversMutex;
    
#ifdef _WIN32
    bool m_winsockInitialized;
#endif
}; 