#include "ServerDiscovery.h"
#include <iostream>
#include <cstring>
#include <algorithm>

ServerDiscovery::ServerDiscovery()
    : m_listenSocket(INVALID_SOCKET)
    , m_running(false)
#ifdef _WIN32
    , m_winsockInitialized(false)
#endif
{
}

ServerDiscovery::~ServerDiscovery() {
    Stop();
}

bool ServerDiscovery::InitializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    m_winsockInitialized = true;
#endif
    return true;
}

void ServerDiscovery::CleanupWinsock() {
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
        m_winsockInitialized = false;
    }
#endif
}

bool ServerDiscovery::Start() {
    if (m_running) {
        return false;
    }
    
    if (!InitializeWinsock()) {
        return false;
    }
    
    // Create UDP socket for receiving broadcasts
    m_listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create discovery listen socket" << std::endl;
        CleanupWinsock();
        return false;
    }
    
    // Enable address reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Enable broadcast receiving
    int broadcast = 1;
#ifdef _WIN32
    if (setsockopt(m_listenSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
#else
    if (setsockopt(m_listenSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
#endif
        std::cerr << "Failed to enable broadcast on discovery socket" << std::endl;
#ifdef _WIN32
        closesocket(m_listenSocket);
#else
        close(m_listenSocket);
#endif
        CleanupWinsock();
        return false;
    }
    
    // Bind to broadcast port
    sockaddr_in listenAddr{};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    listenAddr.sin_port = htons(8081); // Same port as server broadcasts
    
    if (bind(m_listenSocket, (sockaddr*)&listenAddr, sizeof(listenAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind discovery socket to port 8081" << std::endl;
#ifdef _WIN32
        closesocket(m_listenSocket);
#else
        close(m_listenSocket);
#endif
        CleanupWinsock();
        return false;
    }
    
    m_running = true;
    m_listenThread = std::thread(&ServerDiscovery::ListenForBroadcasts, this);
    
    std::cout << "Server discovery started, listening on port 8081" << std::endl;
    return true;
}

void ServerDiscovery::Stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Close listen socket
    if (m_listenSocket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_listenSocket);
#else
        close(m_listenSocket);
#endif
        m_listenSocket = INVALID_SOCKET;
    }
    
    // Wait for listen thread to finish
    if (m_listenThread.joinable()) {
        m_listenThread.join();
    }
    
    CleanupWinsock();
    
    std::cout << "Server discovery stopped" << std::endl;
}

void ServerDiscovery::ListenForBroadcasts() {
    const int CLEANUP_INTERVAL = 30; // Clean up old servers every 30 seconds
    auto lastCleanup = std::chrono::steady_clock::now();
    
    std::cout << "ServerDiscovery: Listening for broadcasts on port 8081..." << std::endl;
    
    while (m_running) {
        // Set socket timeout to avoid blocking indefinitely
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
#ifdef _WIN32
        setsockopt(m_listenSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
        setsockopt(m_listenSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
        
        // Receive broadcast data
        ServerAnnouncement announcement;
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);
        
        ssize_t bytesReceived = recvfrom(m_listenSocket, 
                                        reinterpret_cast<char*>(&announcement), 
                                        sizeof(announcement), 
                                        0, 
                                        reinterpret_cast<sockaddr*>(&fromAddr), 
                                        &fromLen);
        
        if (bytesReceived > 0) {
            char fromIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &fromAddr.sin_addr, fromIP, INET_ADDRSTRLEN);
            
            std::cout << "ServerDiscovery: Received " << bytesReceived 
                      << " bytes from " << fromIP << std::endl;
            
            if (bytesReceived == sizeof(ServerAnnouncement)) {
                // Verify magic bytes
                if (std::memcmp(announcement.magic, "MC_SERVR", 8) == 0) {
                    std::cout << "ServerDiscovery: Valid server announcement from " << fromIP << std::endl;
                    ProcessServerAnnouncement(announcement, std::string(fromIP));
                } else {
                    std::cout << "ServerDiscovery: Invalid magic bytes in packet from " << fromIP << std::endl;
                }
            } else {
                std::cout << "ServerDiscovery: Wrong packet size from " << fromIP 
                          << " (expected " << sizeof(ServerAnnouncement) << ", got " << bytesReceived << ")" << std::endl;
            }
        }
        
        // Periodic cleanup of old servers
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() >= CLEANUP_INTERVAL) {
            CleanupOldServers();
            lastCleanup = now;
        }
    }
    
    std::cout << "ServerDiscovery: Stopped listening for broadcasts" << std::endl;
}

void ServerDiscovery::ProcessServerAnnouncement(const ServerAnnouncement& announcement, const std::string& fromIP) {
    std::lock_guard<std::mutex> lock(m_serversMutex);
    
    // Create server key (IP:PORT)
    std::string serverKey = std::string(announcement.serverIP) + ":" + std::to_string(announcement.serverPort);
    
    // Update or create server entry
    DiscoveredServer& server = m_discoveredServers[serverKey];
    server.name = std::string(announcement.serverName);
    server.ip = std::string(announcement.serverIP);
    server.port = announcement.serverPort;
    server.playerCount = announcement.playerCount;
    server.maxPlayers = announcement.maxPlayers;
    server.lastSeen = std::chrono::steady_clock::now();
    
    std::cout << "Discovered server: " << server.GetDisplayName() << std::endl;
}

std::vector<DiscoveredServer> ServerDiscovery::GetDiscoveredServers() {
    std::lock_guard<std::mutex> lock(m_serversMutex);
    
    std::vector<DiscoveredServer> activeServers;
    for (const auto& pair : m_discoveredServers) {
        if (pair.second.IsActive()) {
            activeServers.push_back(pair.second);
        }
    }
    
    // Sort by server name for consistent display
    std::sort(activeServers.begin(), activeServers.end(), 
              [](const DiscoveredServer& a, const DiscoveredServer& b) {
                  return a.name < b.name;
              });
    
    return activeServers;
}

void ServerDiscovery::CleanupOldServers() {
    std::lock_guard<std::mutex> lock(m_serversMutex);
    
    auto it = m_discoveredServers.begin();
    while (it != m_discoveredServers.end()) {
        if (!it->second.IsActive()) {
            std::cout << "Removing inactive server: " << it->second.GetDisplayName() << std::endl;
            it = m_discoveredServers.erase(it);
        } else {
            ++it;
        }
    }
} 