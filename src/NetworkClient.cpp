#include "NetworkClient.h"
#include <iostream>
#include <sstream>
#include <cstring>

NetworkClient::NetworkClient()
    : m_socket(INVALID_SOCKET)
    , m_connected(false)
    , m_serverPort(8080)
#ifdef _WIN32
    , m_winsockInitialized(false)
#endif
{
}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::InitializeWinsock() {
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

void NetworkClient::CleanupWinsock() {
#ifdef _WIN32
    if (m_winsockInitialized) {
        WSACleanup();
        m_winsockInitialized = false;
    }
#endif
}

bool NetworkClient::Connect(const std::string& serverIP, int port) {
    if (m_connected) {
        return false;
    }
    
    if (!InitializeWinsock()) {
        return false;
    }
    
    m_serverIP = serverIP;
    m_serverPort = port;
    
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        CleanupWinsock();
        return false;
    }
    
    // Connect to server
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address: " << serverIP << std::endl;
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        CleanupWinsock();
        return false;
    }
    
    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server " << serverIP << ":" << port << std::endl;
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        CleanupWinsock();
        return false;
    }
    
    m_connected = true;
    m_receiveThread = std::thread(&NetworkClient::ReceiveMessages, this);
    
    std::cout << "Connected to server " << serverIP << ":" << port << std::endl;
    return true;
}

void NetworkClient::Disconnect() {
    if (!m_connected) {
        return;
    }
    
    m_connected = false;
    
    // Close socket
    if (m_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }
    
    // Wait for receive thread to finish
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
    
    // Clear other players
    {
        std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
        m_otherPlayers.clear();
    }
    
    CleanupWinsock();
    std::cout << "Disconnected from server" << std::endl;
}

void NetworkClient::SendPlayerPosition(const PlayerPosition& position) {
    if (!m_connected) {
        return;
    }
    
    NetworkMessage message;
    message.type = NetworkMessage::PLAYER_POSITION;
    message.playerId = 0; // Server will assign the correct player ID
    message.position = position;
    
    int bytesSent = send(m_socket, (const char*)&message, sizeof(NetworkMessage), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send position update to server" << std::endl;
        // Could trigger disconnect here if needed
    }
}

void NetworkClient::ReceiveMessages() {
    NetworkMessage message;
    
    while (m_connected) {
        int bytesReceived = recv(m_socket, (char*)&message, sizeof(NetworkMessage), 0);
        if (bytesReceived <= 0) {
            // Server disconnected or error
            if (m_connected) {
                std::cerr << "Lost connection to server" << std::endl;
                m_connected = false;
            }
            break;
        }
        
        if (bytesReceived == sizeof(NetworkMessage)) {
            ProcessMessage(message);
        }
    }
    
    // If we get here, connection was lost
    m_connected = false;
}

void NetworkClient::ProcessMessage(const NetworkMessage& message) {
    switch (message.type) {
        case NetworkMessage::PLAYER_JOIN:
        {
            {
                std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
                m_otherPlayers[message.playerId] = message.position;
            }
            
            if (m_onPlayerJoin) {
                m_onPlayerJoin(message.playerId, message.position);
            }
            
            std::cout << "Player " << message.playerId << " joined the game" << std::endl;
            break;
        }
        
        case NetworkMessage::PLAYER_LEAVE:
        {
            // Check for server shutdown (special case: playerId = 0)
            if (message.playerId == 0) {
                std::cout << "Server is shutting down!" << std::endl;
                m_connected = false;
                return;
            }
            
            {
                std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
                m_otherPlayers.erase(message.playerId);
            }
            
            if (m_onPlayerLeave) {
                m_onPlayerLeave(message.playerId);
            }
            
            std::cout << "Player " << message.playerId << " left the game" << std::endl;
            break;
        }
        
        case NetworkMessage::PLAYER_POSITION:
        {
            {
                std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
                m_otherPlayers[message.playerId] = message.position;
            }
            
            if (m_onPlayerPosition) {
                m_onPlayerPosition(message.playerId, message.position);
            }
            break;
        }
        
        case NetworkMessage::PLAYER_LIST:
        {
            {
                std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
                m_otherPlayers[message.playerId] = message.position;
            }
            
            // This is an existing player from when we joined
            std::cout << "Existing player " << message.playerId << " in game" << std::endl;
            break;
        }
        
        case NetworkMessage::WORLD_SEED:
        {
            std::cout << "Received world seed: " << message.worldSeed << std::endl;
            
            if (m_onWorldSeed) {
                m_onWorldSeed(message.worldSeed);
            }
            break;
        }
        
        case NetworkMessage::TIME_SYNC:
        {
            std::cout << "Received game time sync: " << message.gameTime << std::endl;
            
            if (m_onGameTime) {
                m_onGameTime(message.gameTime);
            }
            break;
        }
    }
}

void NetworkClient::SetPlayerJoinCallback(std::function<void(uint32_t, const PlayerPosition&)> callback) {
    m_onPlayerJoin = callback;
}

void NetworkClient::SetPlayerLeaveCallback(std::function<void(uint32_t)> callback) {
    m_onPlayerLeave = callback;
}

void NetworkClient::SetPlayerPositionCallback(std::function<void(uint32_t, const PlayerPosition&)> callback) {
    m_onPlayerPosition = callback;
}

void NetworkClient::SetWorldSeedCallback(std::function<void(int32_t)> callback) {
    m_onWorldSeed = callback;
}

void NetworkClient::SetGameTimeCallback(std::function<void(float)> callback) {
    m_onGameTime = callback;
}

std::unordered_map<uint32_t, PlayerPosition> NetworkClient::GetOtherPlayers() const {
    std::lock_guard<std::mutex> lock(m_otherPlayersMutex);
    return m_otherPlayers;
}

std::string NetworkClient::GetConnectionInfo() const {
    if (!m_connected) {
        return "Not connected";
    }
    
    std::ostringstream oss;
    oss << "Connected to " << m_serverIP << ":" << m_serverPort;
    return oss.str();
} 