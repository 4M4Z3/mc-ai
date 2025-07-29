#include "Server.h"
#include "World.h"
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
    , m_gameTime(0.0f)
    , m_timeUpdating(false)
#ifdef _WIN32
    , m_winsockInitialized(false)
#endif
{
    // Generate server-managed world seed
    m_worldSeed = static_cast<int32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::cout << "Server generated world seed: " << m_worldSeed << std::endl;
    
    // Create world for spawn calculations
    m_world = std::make_unique<World>(m_worldSeed);
    std::cout << "Server world generated for spawn calculations" << std::endl;
    
    // Initialize game time
    m_gameStartTime = std::chrono::steady_clock::now();
    m_lastTimeSyncBroadcast = m_gameStartTime;
    std::cout << "Game time initialized (15 minute day cycle)" << std::endl;
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
    
    // Start time management
    StartTimeSync();
    
    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void Server::Stop() {
    if (!m_running) {
        return;
    }
    
    std::cout << "Server shutting down, notifying all clients..." << std::endl;
    
    // Notify all clients that server is shutting down
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        NetworkMessage shutdownMessage;
        shutdownMessage.type = NetworkMessage::PLAYER_LEAVE; // Reuse existing message type
        shutdownMessage.playerId = 0; // Special ID for server shutdown
        
        for (auto& client : m_clients) {
            if (client && client->active) {
                send(client->socket, (const char*)&shutdownMessage, sizeof(NetworkMessage), 0);
            }
        }
    }
    
    m_running = false;
    
    // Stop UDP broadcast
    StopBroadcast();
    
    // Stop time management
    StopTimeSync();
    
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
        client->position = CalculateSpawnPosition(client->playerId);
        
        // Send player list to new client
        SendPlayerList(clientSocket);
        
        // Send world seed to new client
        SendWorldSeed(clientSocket);
        
        // Send current game time to new client
        SendGameTime(clientSocket);
        
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
            // Handle different message types
            switch (message.type) {
                case NetworkMessage::PLAYER_POSITION:
                {
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
                    break;
                }
                
                case NetworkMessage::BLOCK_BREAK:
                {
                    std::cout << "[SERVER] Player " << playerId << " broke block at (" 
                              << message.blockPos.x << ", " << message.blockPos.y << ", " << message.blockPos.z << ")" << std::endl;
                    
                    // Apply block break to server world
                    if (m_world) {
                        m_world->SetBlock(message.blockPos.x, message.blockPos.y, message.blockPos.z, BlockType::AIR);
                        
                        // Regenerate affected chunk meshes on server (if needed for server-side logic)
                        int chunkX, chunkZ, localX, localZ;
                        m_world->WorldToChunkCoords(message.blockPos.x, message.blockPos.z, chunkX, chunkZ, localX, localZ);
                        
                        Chunk* chunk = m_world->GetChunk(chunkX, chunkZ);
                        if (chunk) {
                            chunk->GenerateMesh(m_world.get());
                        }
                    }
                    
                    // Broadcast block break to all clients (including the sender for confirmation)
                    message.playerId = playerId;
                    BroadcastToAllClients(message);
                    break;
                }
                
                default:
                    std::cerr << "[SERVER] Unknown message type: " << (int)message.type << std::endl;
                    break;
            }
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

void Server::SendWorldSeed(socket_t clientSocket) {
    NetworkMessage seedMessage;
    seedMessage.type = NetworkMessage::WORLD_SEED;
    seedMessage.playerId = 0; // Not relevant for seed message
    seedMessage.worldSeed = m_worldSeed;
    
    int bytesSent = send(clientSocket, (const char*)&seedMessage, sizeof(NetworkMessage), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send world seed to client" << std::endl;
    } else {
        std::cout << "Sent world seed " << m_worldSeed << " to client" << std::endl;
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

PlayerPosition Server::CalculateSpawnPosition(uint32_t playerId) {
    PlayerPosition position;
    position.x = 0.0f;
    position.z = 0.0f;
    position.yaw = 0.0f;
    position.pitch = 0.0f;
    position.playerId = playerId;
    
    // Calculate spawn Y position based on terrain
    if (m_world) {
        position.y = static_cast<float>(m_world->FindHighestBlock(0, 0));
        std::cout << "Calculated spawn position for player " << playerId << " at (0, " << position.y << ", 0)" << std::endl;
    } else {
        position.y = 64.0f; // Fallback height
        std::cout << "Using fallback spawn height for player " << playerId << std::endl;
    }
    
    return position;
}

// Time management methods
void Server::SendGameTime(socket_t clientSocket) {
    NetworkMessage timeMessage;
    timeMessage.type = NetworkMessage::TIME_SYNC;
    timeMessage.playerId = 0; // Not used for time sync
    timeMessage.gameTime = m_gameTime;
    
    int bytesSent = send(clientSocket, (const char*)&timeMessage, sizeof(NetworkMessage), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send game time to client" << std::endl;
    } else {
        std::cout << "Sent game time " << m_gameTime << " to new client" << std::endl;
    }
}

void Server::BroadcastGameTime() {
    NetworkMessage timeMessage;
    timeMessage.type = NetworkMessage::TIME_SYNC;
    timeMessage.playerId = 0; // Not used for time sync
    timeMessage.gameTime = m_gameTime;
    
    int clientCount = 0;
    int successCount = 0;
    
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& client : m_clients) {
        if (client && client->active) {
            clientCount++;
            int bytesSent = send(client->socket, (const char*)&timeMessage, sizeof(NetworkMessage), 0);
            if (bytesSent == SOCKET_ERROR) {
                std::cerr << "[SERVER] Failed to broadcast game time to player " << client->playerId << std::endl;
            } else {
                successCount++;
            }
        }
    }
    
    std::cout << "[SERVER] Broadcasted game time " << m_gameTime << " to " << successCount << "/" << clientCount << " clients" << std::endl;
}

bool Server::IsDay() const {
    return m_gameTime < 450.0f; // First 7.5 minutes (450 seconds) is day
}

bool Server::IsNight() const {
    return m_gameTime >= 450.0f; // Last 7.5 minutes (450-900 seconds) is night
}

void Server::UpdateGameTime() {  
    const float DAY_CYCLE_SECONDS = 900.0f; // 15 minutes in seconds
    const float TIME_SYNC_INTERVAL = 5.0f; // 5 seconds for testing (was 30 seconds)
    
    std::cout << "[SERVER] Time update thread started" << std::endl;
    
    // Send initial time sync immediately when time thread starts
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Brief delay for server to start
    std::cout << "[SERVER] Sending initial time sync..." << std::endl;
    BroadcastGameTime();
    
    auto lastDebugTime = std::chrono::steady_clock::now();
    
    while (m_timeUpdating) {
        auto now = std::chrono::steady_clock::now();
        
        // Calculate elapsed time since game start
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_gameStartTime);
        float totalElapsed = elapsed.count() / 1000.0f; // Convert to seconds
        
        // Update game time (cycles every 15 minutes) - speed up for testing
        float acceleratedElapsed = totalElapsed * 10.0f; // 10x speed for testing
        m_gameTime = fmod(acceleratedElapsed, DAY_CYCLE_SECONDS);
        
        // Debug output every 2 seconds for better visibility
        auto timeSinceDebug = std::chrono::duration_cast<std::chrono::seconds>(now - lastDebugTime);
        if (timeSinceDebug.count() >= 2) {
            std::cout << "[SERVER] Game time: " << m_gameTime << " seconds (elapsed: " << totalElapsed 
                      << "s, " << (IsDay() ? "DAY" : "NIGHT") << ")" << std::endl;
            lastDebugTime = now;
        }
        
        // Check if we need to broadcast time sync
        auto timeSinceLastSync = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastTimeSyncBroadcast);
        if (timeSinceLastSync.count() >= TIME_SYNC_INTERVAL) {
            std::cout << "[SERVER] Broadcasting time sync..." << std::endl;
            BroadcastGameTime();
            m_lastTimeSyncBroadcast = now;
        }
        
        // Sleep for a short time to avoid excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "[SERVER] Time update thread ended" << std::endl;
}

void Server::StartTimeSync() {
    if (m_timeUpdating) {
        std::cout << "Time synchronization already running" << std::endl;
        return; // Already running
    }
    
    std::cout << "[SERVER] Starting time synchronization..." << std::endl;
    m_timeUpdating = true;
    m_timeUpdateThread = std::thread(&Server::UpdateGameTime, this);
    
    std::cout << "[SERVER] Time synchronization thread started successfully" << std::endl;
}

void Server::StopTimeSync() {
    if (!m_timeUpdating) {
        return; // Not running
    }
    
    m_timeUpdating = false;
    
    if (m_timeUpdateThread.joinable()) {
        m_timeUpdateThread.join();
    }
    
    std::cout << "Time synchronization stopped" << std::endl;
} 