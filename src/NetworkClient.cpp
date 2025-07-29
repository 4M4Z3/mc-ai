#include "NetworkClient.h"
#include <iostream>
#include <sstream>
#include <cstring>

NetworkClient::NetworkClient() : m_socket(INVALID_SOCKET), m_connected(false), m_shouldStopSending(false), m_serverPort(8080) {
#ifdef _WIN32
    m_winsockInitialized = false;
#endif
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
    m_shouldStopSending = false;
    m_receiveThread = std::thread(&NetworkClient::ReceiveMessages, this);
    m_sendThread = std::thread(&NetworkClient::SendMessagesThread, this);
    
    std::cout << "[CLIENT] Successfully connected to server " << serverIP << ":" << port << std::endl;
    return true;
}

void NetworkClient::Disconnect() {
    if (!m_connected) {
        return;
    }
    
    m_connected = false;
    m_shouldStopSending = true;
    
    // Wait for send thread to finish
    if (m_sendThread.joinable()) {
        m_sendThread.join();
    }
    
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
    std::cout << "[CLIENT] Disconnected from server " << m_serverIP << ":" << m_serverPort << std::endl;
}

void NetworkClient::SendPlayerPosition(const PlayerPosition& position) {
    if (!m_connected) {
        return;
    }
    
    NetworkMessage message;
    message.type = NetworkMessage::PLAYER_POSITION;
    message.playerId = 0; // Server will assign the correct player ID
    message.position = position;
    
    QueueMessage(message);
}

void NetworkClient::SendBlockBreak(int32_t x, int32_t y, int32_t z) {
    if (!m_connected) {
        return;
    }
    
    NetworkMessage message;
    message.type = NetworkMessage::BLOCK_BREAK;
    message.playerId = 0; // Server will assign the correct player ID
    message.blockPos.x = x;
    message.blockPos.y = y;
    message.blockPos.z = z;
    
    QueueMessage(message);
}

void NetworkClient::RequestChunk(int32_t chunkX, int32_t chunkZ)
{
    if (!m_connected) {
        return;
    }
    
    NetworkMessage message;
    message.type = NetworkMessage::CHUNK_REQUEST;
    message.playerId = 0; // Server will assign the correct player ID
    message.chunkData.chunkX = chunkX;
    message.chunkData.chunkZ = chunkZ;
    
    int bytesSent = send(m_socket, (const char*)&message, sizeof(NetworkMessage), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to request chunk from server" << std::endl;
    } else {
        std::cout << "[CLIENT] Requested chunk (" << chunkX << ", " << chunkZ << ") from server" << std::endl;
    }
}

void NetworkClient::SendMessagesThread() {
    while (m_connected && !m_shouldStopSending) {
        NetworkMessage message;
        bool hasMessage = false;
        
        // Check for outgoing messages
        {
            std::lock_guard<std::mutex> lock(m_outgoingMessagesMutex);
            if (!m_outgoingMessages.empty()) {
                message = m_outgoingMessages.front();
                m_outgoingMessages.pop();
                hasMessage = true;
            }
        }
        
        if (hasMessage) {
            // Send the message
            size_t messageSize = sizeof(NetworkMessage);
            size_t totalBytesSent = 0;
            const char* messageBuffer = reinterpret_cast<const char*>(&message);
            
            while (totalBytesSent < messageSize && m_connected) {
                int bytesSent = send(m_socket, 
                                   messageBuffer + totalBytesSent, 
                                   messageSize - totalBytesSent, 
                                   0);
                
                if (bytesSent == SOCKET_ERROR) {
                    std::cerr << "[CLIENT] Failed to send message type " << (int)message.type << std::endl;
                    break;
                }
                
                totalBytesSent += bytesSent;
            }
        } else {
            // No messages, sleep briefly to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void NetworkClient::QueueMessage(const NetworkMessage& message) {
    if (!m_connected) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_outgoingMessagesMutex);
    m_outgoingMessages.push(message);
}

void NetworkClient::ReceiveMessages() {
    std::cout << "[CLIENT] Starting message receive loop for " << m_serverIP << ":" << m_serverPort << std::endl;
    std::cout << "[CLIENT] NetworkMessage size: " << sizeof(NetworkMessage) << " bytes" << std::endl;
    
    while (m_connected) {
        NetworkMessage message;
        size_t totalBytesReceived = 0;
        size_t messageSize = sizeof(NetworkMessage);
        char* messageBuffer = reinterpret_cast<char*>(&message);
        
        // Receive complete message by handling TCP fragmentation
        while (totalBytesReceived < messageSize && m_connected) {
            int bytesReceived = recv(m_socket, 
                                   messageBuffer + totalBytesReceived, 
                                   messageSize - totalBytesReceived, 
                                   0);
            
            if (bytesReceived <= 0) {
                // Server disconnected or error
                if (m_connected) {
                    std::cerr << "[CLIENT] Lost connection to server " << m_serverIP << ":" << m_serverPort 
                              << " (bytes received: " << bytesReceived << ", total: " << totalBytesReceived 
                              << "/" << messageSize << ")" << std::endl;
                    
                    // Print errno for debugging
#ifdef _WIN32
                    int error = WSAGetLastError();
                    std::cerr << "[CLIENT] Winsock error: " << error << std::endl;
#else
                    std::cerr << "[CLIENT] Socket error: " << strerror(errno) << " (" << errno << ")" << std::endl;
#endif
                    m_connected = false;
                }
                return; // Exit the function completely
            }
            
            totalBytesReceived += bytesReceived;
            
            // Log progress for large messages
            if (messageSize > 1000 && totalBytesReceived < messageSize) {
                std::cout << "[CLIENT] Receiving large message: " << totalBytesReceived 
                          << "/" << messageSize << " bytes" << std::endl;
            }
        }
        
        // Process complete message
        if (totalBytesReceived == messageSize) {
            try {
                ProcessMessage(message);
            } catch (const std::exception& e) {
                std::cerr << "[CLIENT] ERROR processing message: " << e.what() << std::endl;
                // Don't disconnect on message processing errors, just log them
            }
        } else {
            std::cerr << "[CLIENT] Failed to receive complete message: " << totalBytesReceived 
                      << "/" << messageSize << " bytes" << std::endl;
            break;
        }
    }
    
    // If we get here, connection was lost
    std::cout << "[CLIENT] Message receive loop ended for " << m_serverIP << ":" << m_serverPort << std::endl;
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
        
        case NetworkMessage::BLOCK_BREAK:
        {
            std::cout << "Player " << message.playerId << " broke block at (" 
                      << message.blockPos.x << ", " << message.blockPos.y << ", " << message.blockPos.z << ")" << std::endl;
            
            if (m_onBlockBreak) {
                m_onBlockBreak(message.playerId, message.blockPos.x, message.blockPos.y, message.blockPos.z);
            }
            break;
        }
        
        case NetworkMessage::CHUNK_DATA:
        {
            std::cout << "[CLIENT] Received chunk data (" 
                      << message.chunkData.chunkX << ", " << message.chunkData.chunkZ << ")" << std::endl;
            
            if (m_onChunkData) {
                m_onChunkData(message.chunkData.chunkX, message.chunkData.chunkZ, message.chunkData.blocks);
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

void NetworkClient::SetBlockBreakCallback(std::function<void(uint32_t, int32_t, int32_t, int32_t)> callback) {
    m_onBlockBreak = callback;
}

void NetworkClient::SetChunkDataCallback(std::function<void(int32_t, int32_t, const uint8_t*)> callback) {
    m_onChunkData = callback;
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