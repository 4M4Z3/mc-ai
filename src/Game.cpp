#include "Game.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <chrono>
#include <algorithm>
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#endif

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
#endif

// ImGui includes
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// Static instance for callbacks
Game* Game::s_instance = nullptr;

Game::Game() : m_window(nullptr), m_currentState(GameState::MAIN_MENU), m_shouldClose(false),
               m_isHost(false), m_firstMouse(true), m_lastX(640.0), m_lastY(360.0), 
               m_deltaTime(0.0f), m_lastFrame(0.0f) {
    s_instance = this;
}

Game::~Game() {
    Shutdown();
}

bool Game::Initialize(int windowWidth, int windowHeight) {
    // Initialize GLFW
    glfwSetErrorCallback(ErrorCallback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Set OpenGL version (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    m_window = glfwCreateWindow(windowWidth, windowHeight, "ImGui OpenGL Game", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCursorPosCallback(m_window, MouseCallback);
    
    // Start with normal cursor since we begin in main menu
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Enable vsync
    glfwSwapInterval(1);

    // Initialize renderer
    if (!m_renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
#ifdef __APPLE__
    const char* glsl_version = "#version 150";
#else
    const char* glsl_version = "#version 130";
#endif
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Create player
    m_player = std::make_unique<Player>(0.0f, 5.0f, 3.0f);

    // Initialize server discovery
    m_serverDiscovery = std::make_unique<ServerDiscovery>();
    if (!m_serverDiscovery->Start()) {
        std::cerr << "Warning: Failed to start server discovery" << std::endl;
        // Don't fail completely, just continue without server discovery
    }

    std::cout << "Game initialized successfully!" << std::endl;
    return true;
}

void Game::Run() {
    while (!glfwWindowShouldClose(m_window) && !m_shouldClose) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;
        
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ProcessInput();

        // Update based on current state
        switch (m_currentState) {
            case GameState::MAIN_MENU:
                UpdateMainMenu();
                break;
            case GameState::GAME:
                UpdateGame();
                break;
        }

        // Render
        m_renderer.Clear();
        
        switch (m_currentState) {
            case GameState::MAIN_MENU:
                RenderMainMenu();
                break;
            case GameState::GAME:
                RenderGame();
                break;
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void Game::Shutdown() {
    // Stop server discovery
    if (m_serverDiscovery) {
        m_serverDiscovery->Stop();
        m_serverDiscovery.reset();
    }
    
    if (m_window) {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_renderer.Shutdown();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
}

void Game::ProcessInput() {
    // ESC key handling is done in KeyCallback
    
    // Player movement (only in game state)
    if (m_currentState == GameState::GAME && m_player) {
        m_player->ProcessInput(m_window, m_deltaTime);
    }
}

void Game::SetState(GameState newState) {
    m_currentState = newState;
    std::cout << "State changed to: " << (newState == GameState::MAIN_MENU ? "MAIN_MENU" : "GAME") << std::endl;
    
    // Handle cursor visibility and mouse capture
    if (newState == GameState::MAIN_MENU) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_firstMouse = true; // Reset mouse for when we return to game
    } else if (newState == GameState::GAME) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Create world when entering game state
        if (!m_world) {
            m_world = std::make_unique<World>();
        }
    }
}

void Game::UpdateMainMenu() {
    // Main menu logic here
}

void Game::UpdateGame() {
    // Send player position updates to server
    static float lastPositionSend = 0.0f;
    const float positionSendInterval = 1.0f / 20.0f; // Send 20 times per second
    
    if (glfwGetTime() - lastPositionSend > positionSendInterval) {
        SendPlayerPosition();
        lastPositionSend = glfwGetTime();
    }
}

void Game::RenderMainMenu() {
    // Create a centered window for the main menu with increased size for server list
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Always);
    
    if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Minecraft Clone - Multiplayer");
        ImGui::Separator();
        
        if (ImGui::Button("Host Game", ImVec2(580, 50))) {
            StartHost();
        }
        
        ImGui::Separator();
        
        // Available Servers Section
        ImGui::Text("Available Servers:");
        if (m_serverDiscovery) {
            std::vector<DiscoveredServer> discoveredServers = m_serverDiscovery->GetDiscoveredServers();
            
            if (!discoveredServers.empty()) {
                // Create a child window with scrolling for the server list
                ImGui::BeginChild("ServerList", ImVec2(0, 120), true);
                
                for (const auto& server : discoveredServers) {
                    std::string buttonLabel = server.GetDisplayName();
                    if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 30))) {
                        JoinServer(server.ip + ":" + std::to_string(server.port));
                    }
                }
                
                ImGui::EndChild();
            } else {
                ImGui::Text("  No servers found on local network");
                ImGui::Text("  Servers will appear here automatically");
            }
        } else {
            ImGui::Text("  Server discovery not available");
        }
        
        ImGui::Separator();
        
        // Manual server entry (kept as backup option)
        static char serverIP[128] = "127.0.0.1";
        ImGui::Text("Manual Server Entry:");
        ImGui::InputText("Server IP", serverIP, sizeof(serverIP));
        
        if (ImGui::Button("Join Manually", ImVec2(580, 50))) {
            JoinServer(std::string(serverIP));
        }
        
        // Network debugging section
        ImGui::Separator();
        ImGui::Text("Network Debugging:");
        
        if (ImGui::Button("Test UDP to Server", ImVec2(280, 30))) {
            TestUDPConnectivity(std::string(serverIP));
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Server List", ImVec2(280, 30))) {
            if (m_serverDiscovery) {
                m_serverDiscovery->CleanupOldServers();
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Exit", ImVec2(580, 50))) {
            m_shouldClose = true;
        }
        
        // Show networking status
        if (m_server && m_server->IsRunning()) {
            ImGui::Separator();
            ImGui::Text("Hosting Server:");
            ImGui::Text("  %s", m_server->GetServerInfo().c_str());
            ImGui::Text("  ");
            ImGui::Text("Others can join using:");
            ImGui::Text("  IP: %s", m_server->GetLocalIPAddress().c_str());
            ImGui::Text("  Port: 8080");
        }
        
        if (m_networkClient && m_networkClient->IsConnected()) {
            ImGui::Separator();
            ImGui::Text("Connected: %s", m_networkClient->GetConnectionInfo().c_str());
        }
    }
    ImGui::End();
}

void Game::RenderGame() {
    // 3D world rendering
    if (m_world && m_player) {
        m_renderer.BeginFrame(*m_player);
        m_renderer.RenderWorld(*m_world);
        
        // Render other players with interpolated positions
        if (m_networkClient && m_networkClient->IsConnected()) {
            auto interpolatedPositions = GetInterpolatedPlayerPositions();
            m_renderer.RenderOtherPlayers(interpolatedPositions);
        }
        
        m_renderer.EndFrame();
    }
    
    // Show game UI
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_Always);
    
    if (ImGui::Begin("Minecraft Clone", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("3D Block Renderer");
        ImGui::Separator();
        
        if (m_player) {
            Vec3 pos = m_player->GetPosition();
            ImGui::Text("Player Position:");
            ImGui::Text("  X: %.1f, Y: %.1f, Z: %.1f", pos.x, pos.y, pos.z);
            ImGui::Text("Yaw: %.1f, Pitch: %.1f", m_player->GetYaw(), m_player->GetPitch());
        }
        
        ImGui::Separator();
        
        if (m_networkClient && m_networkClient->IsConnected()) {
            ImGui::Text("Connected Players: %zu", m_otherPlayers.size() + 1); // +1 for self
            
            // Show other players with interpolation info
            for (const auto& pair : m_otherPlayers) {
                const InterpolatedPlayer& player = pair.second;
                PlayerPosition interpPos = player.GetInterpolatedPosition();
                
                ImGui::Text("Player %u:", pair.first);
                ImGui::Text("  Pos: %.1f, %.1f, %.1f", interpPos.x, interpPos.y, interpPos.z);
                
                // Show interpolation debug info
                auto now = std::chrono::steady_clock::now();
                auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(now - player.lastUpdateTime).count();
                ImGui::Text("  Last update: %ldms ago", timeSinceUpdate);
            }
        } else {
            ImGui::Text("Single Player Mode");
        }
        
        if (ImGui::Button("Back to Menu", ImVec2(280, 50))) {
            SetState(GameState::MAIN_MENU);
        }
    }
    ImGui::End();
}

std::unordered_map<uint32_t, PlayerPosition> Game::GetInterpolatedPlayerPositions() const {
    std::unordered_map<uint32_t, PlayerPosition> interpolatedPositions;
    
    for (const auto& pair : m_otherPlayers) {
        interpolatedPositions[pair.first] = pair.second.GetInterpolatedPosition();
    }
    
    return interpolatedPositions;
}

// Static callbacks
void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (s_instance) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME) {
                s_instance->SetState(GameState::MAIN_MENU);
            }
        }
    }
}

void Game::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance && s_instance->m_currentState == GameState::GAME && s_instance->m_player) {
        if (s_instance->m_firstMouse) {
            s_instance->m_lastX = xpos;
            s_instance->m_lastY = ypos;
            s_instance->m_firstMouse = false;
        }
        
        double xoffset = xpos - s_instance->m_lastX;
        double yoffset = s_instance->m_lastY - ypos; // Reversed since y-coordinates go from bottom to top
        
        s_instance->m_lastX = xpos;
        s_instance->m_lastY = ypos;
        
        s_instance->m_player->ProcessMouseMovement(xoffset, yoffset);
    }
}

void Game::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (s_instance) {
        s_instance->m_renderer.SetViewport(width, height);
    }
}

void Game::ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Networking methods
void Game::StartHost() {
    m_server = std::make_unique<Server>();
    if (m_server->Start(8080)) {
        m_isHost = true;
        
        // Also connect as a client to our own server
        m_networkClient = std::make_unique<NetworkClient>();
        
        // Set up callbacks
        m_networkClient->SetPlayerJoinCallback([this](uint32_t playerId, const PlayerPosition& position) {
            OnPlayerJoin(playerId, position);
        });
        
        m_networkClient->SetPlayerLeaveCallback([this](uint32_t playerId) {
            OnPlayerLeave(playerId);
        });
        
        m_networkClient->SetPlayerPositionCallback([this](uint32_t playerId, const PlayerPosition& position) {
            OnPlayerPositionUpdate(playerId, position);
        });
        
        if (m_networkClient->Connect("127.0.0.1", 8080)) {
            // Create world and enter game
            m_world = std::make_unique<World>();
            SetState(GameState::GAME);
            std::cout << "Hosting game on port 8080" << std::endl;
        } else {
            std::cerr << "Failed to connect to own server" << std::endl;
            m_server.reset();
            m_networkClient.reset();
            m_isHost = false;
        }
    } else {
        std::cerr << "Failed to start server" << std::endl;
        m_server.reset();
    }
}

void Game::JoinServer(const std::string& serverIP) {
    m_networkClient = std::make_unique<NetworkClient>();
    
    // Parse IP address and port
    std::string ip = serverIP;
    int port = 8080; // Default port
    
    // Check if port is specified in the format IP:PORT
    size_t colonPos = serverIP.find(':');
    if (colonPos != std::string::npos) {
        ip = serverIP.substr(0, colonPos);
        try {
            port = std::stoi(serverIP.substr(colonPos + 1));
        } catch (const std::exception&) {
            std::cerr << "Invalid port number in: " << serverIP << std::endl;
            std::cerr << "Using default port 8080" << std::endl;
            port = 8080;
        }
    }
    
    std::cout << "Attempting to connect to " << ip << ":" << port << std::endl;
    
    // Set up callbacks
    m_networkClient->SetPlayerJoinCallback([this](uint32_t playerId, const PlayerPosition& position) {
        OnPlayerJoin(playerId, position);
    });
    
    m_networkClient->SetPlayerLeaveCallback([this](uint32_t playerId) {
        OnPlayerLeave(playerId);
    });
    
    m_networkClient->SetPlayerPositionCallback([this](uint32_t playerId, const PlayerPosition& position) {
        OnPlayerPositionUpdate(playerId, position);
    });
    
    if (m_networkClient->Connect(ip, port)) {
        // Create world and enter game  
        m_world = std::make_unique<World>();
        SetState(GameState::GAME);
        std::cout << "Joined server at " << ip << ":" << port << std::endl;
    } else {
        std::cerr << "Failed to connect to server: " << ip << ":" << port << std::endl;
        std::cerr << "Make sure:" << std::endl;
        std::cerr << "  1. The server is running on " << ip << std::endl;
        std::cerr << "  2. Port " << port << " is not blocked by firewall" << std::endl;
        std::cerr << "  3. You're on the same network" << std::endl;
        m_networkClient.reset();
    }
}

void Game::SendPlayerPosition() {
    if (m_networkClient && m_networkClient->IsConnected() && m_player) {
        Vec3 pos = m_player->GetPosition();
        PlayerPosition playerPos;
        playerPos.x = pos.x;
        playerPos.y = pos.y;
        playerPos.z = pos.z;
        playerPos.yaw = m_player->GetYaw();
        playerPos.pitch = m_player->GetPitch();
        playerPos.playerId = 0; // Server will assign
        
        m_networkClient->SendPlayerPosition(playerPos);
    }
}

void Game::TestUDPConnectivity(const std::string& targetIP) {
    std::cout << "Testing UDP connectivity to " << targetIP << "..." << std::endl;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock for UDP test" << std::endl;
        return;
    }
#endif
    
    // Create UDP socket
    socket_t testSocket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if (testSocket == INVALID_SOCKET) {
#else
    if (testSocket == -1) {
#endif
        std::cerr << "Failed to create UDP test socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    
    // Set up target address
    sockaddr_in targetAddr{};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(8081); // Server discovery port
    
    if (inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr) <= 0) {
        std::cerr << "Invalid target IP address: " << targetIP << std::endl;
#ifdef _WIN32
        closesocket(testSocket);
        WSACleanup();
#else
        close(testSocket);
#endif
        return;
    }
    
    // Create test message
    const char* testMessage = "UDP_TEST_MESSAGE";
    
    // Send test packet
    ssize_t bytesSent = sendto(testSocket, 
                              testMessage, 
                              strlen(testMessage), 
                              0, 
                              reinterpret_cast<const sockaddr*>(&targetAddr), 
                              sizeof(targetAddr));
    
    if (bytesSent == -1) {
        std::cerr << "Failed to send UDP test packet to " << targetIP << std::endl;
    } else {
        std::cout << "Sent UDP test packet (" << bytesSent << " bytes) to " 
                  << targetIP << ":8081" << std::endl;
        std::cout << "Check the server console to see if it received the packet." << std::endl;
    }
    
    // Clean up
#ifdef _WIN32
    closesocket(testSocket);
    WSACleanup();
#else
    close(testSocket);
#endif
}

void Game::OnPlayerJoin(uint32_t playerId, const PlayerPosition& position) {
    // Create new interpolated player
    InterpolatedPlayer& player = m_otherPlayers[playerId];
    player.currentPos = position;
    player.previousPos = position; // Start with same position to avoid interpolation artifacts
    player.lastUpdateTime = std::chrono::steady_clock::now();
    player.previousUpdateTime = player.lastUpdateTime;
    
    std::cout << "Player " << playerId << " joined at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void Game::OnPlayerLeave(uint32_t playerId) {
    m_otherPlayers.erase(playerId);
    std::cout << "Player " << playerId << " left the game" << std::endl;
}

void Game::OnPlayerPositionUpdate(uint32_t playerId, const PlayerPosition& position) {
    auto it = m_otherPlayers.find(playerId);
    if (it != m_otherPlayers.end()) {
        // Update existing player with smooth interpolation
        it->second.UpdatePosition(position);
    } else {
        // Player doesn't exist yet, create them (shouldn't happen normally)
        OnPlayerJoin(playerId, position);
    }
} 