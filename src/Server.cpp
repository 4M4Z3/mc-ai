#include "Server.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <thread>

Server::Server() 
    : m_serverSocket(INVALID_SOCKET)
    , m_running(false)
    , m_nextPlayerId(1)
    , m_port(8080)
    , m_broadcastSocket(INVALID_SOCKET)
    , m_broadcasting(false)
#ifdef _WIN32
    , m_winsockInitialized(false)
#endif
{
}

Server::~Server() {
    Stop();
}

bool Server::InitializeWinsock() {
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

void Server::CleanupWinsock() {
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
        m_winsockInitialized = false;
    }
#endif
}

bool Server::Start(int port) {
    if (m_running) {
        return false;
    }
    
    if (!InitializeWinsock()) {
        return false;
    }
    
    m_port = port;
    
    // Create socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        CleanupWinsock();
        return false;
    }
    
    // Enable address reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind socket
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
#ifdef _WIN32
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        CleanupWinsock();
        return false;
    }
    
    // Listen for connections
    if (listen(m_serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on socket" << std::endl;
#ifdef _WIN32
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        CleanupWinsock();
        return false;
    }
    
    m_running = true;
    m_acceptThread = std::thread(&Server::AcceptClients, this);
    
    // Start UDP broadcast for server discovery
    StartBroadcast();
    
    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void Server::Stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Stop UDP broadcast
    StopBroadcast();
    
    // Close server socket to stop accepting new connections
    if (m_serverSocket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        m_serverSocket = INVALID_SOCKET;
    }
    
    // Wait for accept thread to finish
    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& client : m_clients) {
            if (client && client->active) {
                client->active = false;
#ifdef _WIN32
                closesocket(client->socket);
#else
                close(client->socket);
#endif
                if (client->thread.joinable()) {
                    client->thread.join();
                }
            }
        }
        m_clients.clear();
    }
    
    CleanupWinsock();
    std::cout << "Server stopped" << std::endl;
}

void Server::AcceptClients() {
    while (m_running) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        socket_t clientSocket = accept(m_serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            if (m_running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        if (!m_running) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            break;
        }
        
        // Create new client info
        auto client = std::make_unique<ClientInfo>();
        client->socket = clientSocket;
        client->playerId = m_nextPlayerId++;
        client->active = true;
        client->position = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, client->playerId};
        
        // Send player list to new client
        SendPlayerList(clientSocket);
        
        // Notify all clients about new player
        NetworkMessage joinMessage;
        joinMessage.type = NetworkMessage::PLAYER_JOIN;
        joinMessage.playerId = client->playerId;
        joinMessage.position = client->position;
        BroadcastToAllClients(joinMessage);
        
        // Start client handler thread
        uint32_t playerId = client->playerId;
        client->thread = std::thread(&Server::HandleClient, this, clientSocket, playerId);
        
        // Add to client list
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients.push_back(std::move(client));
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) 
                  << " (Player ID: " << playerId << ")" << std::endl;
    }
}

void Server::HandleClient(socket_t clientSocket, uint32_t playerId) {
    NetworkMessage message;
    
    while (m_running) {
        // Receive message from client
        int bytesReceived = recv(clientSocket, (char*)&message, sizeof(NetworkMessage), 0);
        if (bytesReceived <= 0) {
            // Client disconnected or error
            break;
        }
        
        if (bytesReceived == sizeof(NetworkMessage)) {
            // Update player position
            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                for (auto& client : m_clients) {
                    if (client && client->playerId == playerId && client->active) {
                        client->position = message.position;
                        client->position.playerId = playerId; // Ensure correct player ID
                        break;
                    }
                }
            }
            
            // Broadcast position update to all other clients
            message.playerId = playerId;
            BroadcastToAllClients(message, playerId);
        }
    }
    
    // Client disconnected - notify other clients and clean up
    NetworkMessage leaveMessage;
    leaveMessage.type = NetworkMessage::PLAYER_LEAVE;
    leaveMessage.playerId = playerId;
    BroadcastToAllClients(leaveMessage, playerId);
    
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (*it && (*it)->playerId == playerId) {
                (*it)->active = false;
#ifdef _WIN32
                closesocket((*it)->socket);
#else
                close((*it)->socket);
#endif
                m_clients.erase(it);
                break;
            }
        }
    }
    
    std::cout << "Player " << playerId << " disconnected" << std::endl;
}

void Server::BroadcastToAllClients(const NetworkMessage& message, uint32_t excludePlayerId) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    for (auto& client : m_clients) {
        if (client && client->active && client->playerId != excludePlayerId) {
            send(client->socket, (const char*)&message, sizeof(NetworkMessage), 0);
        }
    }
}

void Server::SendPlayerList(socket_t clientSocket) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    // Send existing players to new client
    for (auto& client : m_clients) {
        if (client && client->active) {
            NetworkMessage playerMessage;
            playerMessage.type = NetworkMessage::PLAYER_LIST;
            playerMessage.playerId = client->playerId;
            playerMessage.position = client->position;
            
            send(clientSocket, (const char*)&playerMessage, sizeof(NetworkMessage), 0);
        }
    }
}

int Server::GetPlayerCount() {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return static_cast<int>(m_clients.size());
}

std::string Server::GetServerInfo() {
    std::ostringstream oss;
    std::string localIP = GetLocalIPAddress();
    oss << "Server running on " << localIP << ":" << m_port << " with " << GetPlayerCount() << " players connected";
    return oss.str();
}

std::string Server::GetLocalIPAddress() {
#ifdef _WIN32
    // Windows implementation
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        return "127.0.0.1";
    }
    
    struct hostent* host_entry = gethostbyname(hostname);
    if (host_entry == nullptr) {
        return "127.0.0.1";
    }
    
    // Get the first IP address
    char* ip = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    return std::string(ip);
#else
    // Unix/macOS implementation
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return "127.0.0.1";
    }
    
    // Connect to a remote address to determine which interface would be used
    // This doesn't actually connect, just determines the route
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr); // Google DNS
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return "127.0.0.1";
    }
    
    // Get the local address of the socket
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sockfd, (struct sockaddr*)&local_addr, &addr_len) == -1) {
        close(sockfd);
        return "127.0.0.1";
    }
    
    close(sockfd);
    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str);
#endif
}

void Server::StartBroadcast() {
    if (m_broadcasting) {
        return;
    }
    
    // Create UDP socket for broadcasting
    m_broadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_broadcastSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create broadcast socket" << std::endl;
        return;
    }
    
    // Enable broadcast on socket
    int broadcast = 1;
#ifdef _WIN32
    if (setsockopt(m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
#else
    if (setsockopt(m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
#endif
        std::cerr << "Failed to enable broadcast on socket" << std::endl;
#ifdef _WIN32
        closesocket(m_broadcastSocket);
#else
        close(m_broadcastSocket);
#endif
        m_broadcastSocket = INVALID_SOCKET;
        return;
    }
    
    m_broadcasting = true;
    m_broadcastThread = std::thread(&Server::BroadcastServerPresence, this);
    
    std::cout << "Server broadcast started" << std::endl;
}

void Server::StopBroadcast() {
    if (!m_broadcasting) {
        return;
    }
    
    m_broadcasting = false;
    
    // Close broadcast socket
    if (m_broadcastSocket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_broadcastSocket);
#else
        close(m_broadcastSocket);
#endif
        m_broadcastSocket = INVALID_SOCKET;
    }
    
    // Wait for broadcast thread to finish
    if (m_broadcastThread.joinable()) {
        m_broadcastThread.join();
    }
    
    std::cout << "Server broadcast stopped" << std::endl;
}

void Server::BroadcastServerPresence() {
    const int BROADCAST_PORT = 8081; // Different port for discovery
    const int BROADCAST_INTERVAL = 3; // Broadcast every 3 seconds
    
    while (m_broadcasting && m_running) {
        // Create server announcement
        ServerAnnouncement announcement;
        std::strncpy(announcement.serverName, "Minecraft Clone Server", sizeof(announcement.serverName) - 1);
        
        std::string localIP = GetLocalIPAddress();
        std::strncpy(announcement.serverIP, localIP.c_str(), sizeof(announcement.serverIP) - 1);
        
        announcement.serverPort = static_cast<uint16_t>(m_port);
        announcement.playerCount = static_cast<uint16_t>(GetPlayerCount());
        announcement.maxPlayers = 10; // Default max players
        announcement.timestamp = static_cast<uint32_t>(std::time(nullptr));
        
        // Calculate subnet broadcast address
        std::string broadcastIP = GetBroadcastAddress(localIP);
        
        // Set up broadcast address
        sockaddr_in broadcastAddr{};
        broadcastAddr.sin_family = AF_INET;
        broadcastAddr.sin_port = htons(BROADCAST_PORT);
        
        // Try both general broadcast and subnet-specific broadcast
        std::vector<std::string> broadcastAddresses = {
            "255.255.255.255",  // General broadcast
            broadcastIP          // Subnet broadcast
        };
        
        for (const std::string& addr : broadcastAddresses) {
            if (inet_pton(AF_INET, addr.c_str(), &broadcastAddr.sin_addr) <= 0) {
                continue;
            }
            
            // Send broadcast
            ssize_t bytesSent = sendto(m_broadcastSocket, 
                                       reinterpret_cast<const char*>(&announcement), 
                                       sizeof(announcement), 
                                       0, 
                                       reinterpret_cast<const sockaddr*>(&broadcastAddr), 
                                       sizeof(broadcastAddr));
            
            if (bytesSent == -1) {
                if (m_broadcasting) {
                    std::cerr << "Failed to send broadcast to " << addr << std::endl;
                }
            } else {
                std::cout << "Broadcasted to " << addr << ":" << BROADCAST_PORT 
                          << " - Server: " << localIP << ":" << m_port 
                          << " (" << GetPlayerCount() << " players)" << std::endl;
            }
        }
        
        // Wait before next broadcast
        std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
    }
}

std::string Server::GetBroadcastAddress(const std::string& localIP) {
    // Parse the local IP to determine the broadcast address
    // For most home networks (192.168.x.x), use 192.168.x.255
    // For 10.x.x.x networks, use 10.x.x.255 or 10.255.255.255
    
    size_t lastDot = localIP.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string networkPart = localIP.substr(0, lastDot);
        return networkPart + ".255";
    }
    
    // Fallback to general broadcast
    return "255.255.255.255";
} 