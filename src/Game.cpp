#include "Game.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <csignal>
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
               m_isHost(false), m_worldSeed(0), m_worldSeedReceived(false),
               m_firstMouse(true), m_lastX(640.0), m_lastY(360.0), 
               m_deltaTime(0.0f), m_lastFrame(0.0f), m_renderProgressionTimer(0.0f), m_currentRenderMode(0),
               m_purpleTintTimer(0.0f), m_showPauseMenu(false) {
    s_instance = this;
}

Game::~Game() {
    std::cout << "Game destructor called" << std::endl;
    Shutdown();
}

bool Game::Initialize(int windowWidth, int windowHeight) {
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, SignalHandler);  // Ctrl+C
    std::signal(SIGTERM, SignalHandler); // Termination signal
    
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
    glfwSetWindowCloseCallback(m_window, WindowCloseCallback);
    
    // Start with normal cursor since we begin in main menu
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Enable vsync
    glfwSwapInterval(1);

    // Initialize renderer
    if (!m_renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Get actual framebuffer size (important for Retina displays on macOS)
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    m_renderer.SetViewport(framebufferWidth, framebufferHeight);
    
    std::cout << "Window size: " << windowWidth << "x" << windowHeight << std::endl;
    std::cout << "Framebuffer size: " << framebufferWidth << "x" << framebufferHeight << std::endl;

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
    std::cout << "Game shutting down..." << std::endl;
    
    // Disconnect from network first (if client)
    if (m_networkClient) {
        std::cout << "Disconnecting network client..." << std::endl;
        m_networkClient->Disconnect();
        m_networkClient.reset();
    }
    
    // Stop server (if hosting) - this will kick all connected players
    if (m_server && m_isHost) {
        std::cout << "Stopping server..." << std::endl;
        m_server->Stop();
        m_server.reset();
        m_isHost = false;
    }
    
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
    
    std::cout << "Game shutdown complete." << std::endl;
}

void Game::ProcessInput() {
    // ESC key handling is done in KeyCallback
    
    // Player movement (only in game state and not paused)
    if (m_currentState == GameState::GAME && m_player && !m_showPauseMenu) {
        m_player->ProcessInput(m_window, m_deltaTime, m_world.get());
    }
}

void Game::SetState(GameState newState) {
    GameState oldState = m_currentState;
    m_currentState = newState;
    std::cout << "State changed to: " << (newState == GameState::MAIN_MENU ? "MAIN_MENU" : "GAME") << std::endl;
    
    // Reset pause menu when changing states
    m_showPauseMenu = false;
    
    // Handle state transitions
    if (oldState == GameState::GAME && newState == GameState::MAIN_MENU) {
        // Leaving game - clean up networking but keep server discovery
        if (m_networkClient) {
            std::cout << "Disconnecting from game..." << std::endl;
            m_networkClient->Disconnect();
            m_networkClient.reset();
        }
        
        // If we were hosting, stop the server
        if (m_server && m_isHost) {
            std::cout << "Stopping hosted server..." << std::endl;
            m_server->Stop();
            m_server.reset();
            m_isHost = false;
        }
        
        // Clear other players
        m_otherPlayers.clear();
    }
    
    // Handle cursor visibility and mouse capture
    if (newState == GameState::MAIN_MENU) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_firstMouse = true; // Reset mouse for when we return to game
    } else if (newState == GameState::GAME) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Initialize player if we have a world but no player yet
        if (m_world && !m_player) {
            float spawnY = static_cast<float>(m_world->FindHighestBlock(0, 0));
            m_player = std::make_unique<Player>(0.0f, spawnY, 0.0f);
        }
    }
}

void Game::UpdateMainMenu() {
    // Check if we received a world seed and need to create the world
    if (m_worldSeedReceived && !m_world) {
        std::cout << "Creating world with seed " << m_worldSeed << " in main thread..." << std::endl;
        
        // Create world with server-provided seed
        m_world = std::make_unique<World>(m_worldSeed);
        std::cout << "World created successfully!" << std::endl;
        
        // Create player immediately after world creation
        if (!m_player) {
            std::cout << "Creating player at spawn position..." << std::endl;
            float spawnY = static_cast<float>(m_world->FindHighestBlock(0, 0));
            m_player = std::make_unique<Player>(0.0f, spawnY, 0.0f);
            std::cout << "Player created at spawn position (0, " << spawnY << ", 0)" << std::endl;
        }
        
        std::cout << "Setting state to GAME..." << std::endl;
        SetState(GameState::GAME);
        std::cout << "World created with server seed, entering game!" << std::endl;
    }
    
    // Main menu logic here
}

void Game::UpdateGame() {
    // Update player physics (gravity, etc.)
    if (m_player && m_world) {
        m_player->Update(m_deltaTime, m_world.get());
    }
    
    // Update render progression timer
    m_renderProgressionTimer += m_deltaTime;
    
    // Update purple tint timer (starts counting when we reach FULL_RENDER mode)
    if (m_currentRenderMode == 2) { // FULL_RENDER mode
        m_purpleTintTimer += m_deltaTime;
    }
    
    // Calculate smooth transitions with fade periods
    int newRenderMode = 0;
    float fadeFactor = 0.0f;
    
    const float fadeTime = 1.0f; // 1 second fade between modes
    
    if (m_renderProgressionTimer <= 4.0f) {
        // Pure WHITE_ONLY phase (0-4 seconds)
        newRenderMode = 0;
        fadeFactor = 0.0f;
    } else if (m_renderProgressionTimer <= 5.0f) {
        // Fading from WHITE_ONLY to AO_ONLY (4-5 seconds)
        newRenderMode = 0; // Still in WHITE_ONLY mode for shader logic
        fadeFactor = (m_renderProgressionTimer - 4.0f) / fadeTime; // 0.0 to 1.0
    } else if (m_renderProgressionTimer <= 9.0f) {
        // Pure AO_ONLY phase (5-9 seconds)
        newRenderMode = 1;
        fadeFactor = 0.0f;
    } else if (m_renderProgressionTimer <= 10.0f) {
        // Fading from AO_ONLY to FULL_RENDER (9-10 seconds)
        newRenderMode = 1; // Still in AO_ONLY mode for shader logic
        fadeFactor = (m_renderProgressionTimer - 9.0f) / fadeTime; // 0.0 to 1.0
    } else {
        // Pure FULL_RENDER phase (10+ seconds)
        newRenderMode = 2;
        fadeFactor = 0.0f;
    }
    
    // Update renderer if mode changed
    if (newRenderMode != m_currentRenderMode) {
        m_currentRenderMode = newRenderMode;
        m_renderer.SetRenderMode(static_cast<RenderMode>(m_currentRenderMode));
        
        // Log the mode change
        const char* modeNames[] = {"WHITE_ONLY", "AO_ONLY", "FULL_RENDER"};
        std::cout << "Render mode changed to: " << modeNames[m_currentRenderMode] << std::endl;
    }
    
    // Always update fade factor for smooth transitions
    m_renderer.SetFadeFactor(fadeFactor);
    
    // Enable purple tint after 2 seconds in FULL_RENDER mode
    bool enablePurpleTint = (m_currentRenderMode == 2 && m_purpleTintTimer >= 2.0f);
    m_renderer.SetEnablePurpleTint(enablePurpleTint);
    
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
    
    // Show pause menu overlay if active
    if (m_showPauseMenu) {
        RenderPauseMenu();
        return; // Don't show game UI when paused
    }
    
    // Debug UI disabled for clean rendering demonstration
    /*
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
                ImGui::Text("  Last update: %ldms ago", static_cast<long>(timeSinceUpdate));
            }
        } else {
            ImGui::Text("Single Player Mode");
        }
        
        if (ImGui::Button("Back to Menu", ImVec2(280, 50))) {
            SetState(GameState::MAIN_MENU);
        }
    }
    */
    // ImGui::End(); // Commented out since ImGui::Begin is also commented out
}

void Game::RenderPauseMenu() {
    // Create a semi-transparent background overlay
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    // Create a translucent background window
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::Begin("PauseBackground", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        // Empty window just for background
    }
    ImGui::End();
    ImGui::PopStyleColor();
    
    // Create the actual pause menu in the center
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
    
    if (ImGui::Begin("Game Paused", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        // Center the content
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Game Paused").x) * 0.5f);
        ImGui::Text("Game Paused");
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Center the buttons
        float buttonWidth = 300.0f;
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        
        if (ImGui::Button("Resume Game", ImVec2(buttonWidth, 40))) {
            m_showPauseMenu = false;
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        
        ImGui::Spacing();
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
        
        if (ImGui::Button("Back to Main Menu", ImVec2(buttonWidth, 40))) {
            // Hide pause menu and use existing state transition logic
            m_showPauseMenu = false;
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
                // Toggle pause menu instead of immediately going to main menu
                s_instance->m_showPauseMenu = !s_instance->m_showPauseMenu;
                
                // Update cursor visibility based on pause state
                if (s_instance->m_showPauseMenu) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
        }
        
        // Toggle survival mode with backslash key
        if (key == GLFW_KEY_BACKSLASH && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME && s_instance->m_player && s_instance->m_world) {
                s_instance->m_player->ToggleSurvivalMode(s_instance->m_world.get());
                std::cout << "Survival mode: " << (s_instance->m_player->IsSurvivalMode() ? "ON" : "OFF") << std::endl;
            }
        }
        
        // Toggle static camera mode with C key
        if (key == GLFW_KEY_C && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME && s_instance->m_player) {
                s_instance->m_player->ToggleStaticCameraMode();
                std::cout << "Static camera mode: " << (s_instance->m_player->IsStaticCameraMode() ? "ON" : "OFF") << std::endl;
            }
        }
    }
}

void Game::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance && s_instance->m_currentState == GameState::GAME && s_instance->m_player && !s_instance->m_showPauseMenu) {
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

void Game::WindowCloseCallback(GLFWwindow* window) {
    if (s_instance) {
        s_instance->m_shouldClose = true;
    }
}

void Game::SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (s_instance) {
        s_instance->m_shouldClose = true;
    }
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

        m_networkClient->SetWorldSeedCallback([this](int32_t worldSeed) {
            OnWorldSeedReceived(worldSeed);
        });
        
        if (m_networkClient->Connect("127.0.0.1", 8080)) {
            // Wait for world seed from server before creating world
            std::cout << "Connected to own server, waiting for world seed..." << std::endl;
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

    m_networkClient->SetWorldSeedCallback([this](int32_t worldSeed) {
        OnWorldSeedReceived(worldSeed);
    });
    
    if (m_networkClient->Connect(ip, port)) {
        // Wait for world seed from server before creating world
        std::cout << "Connected to server " << ip << ":" << port << ", waiting for world seed..." << std::endl;
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
        it->second.UpdatePosition(position);
    }
}

void Game::OnWorldSeedReceived(int32_t worldSeed) {
    std::cout << "Received world seed from server: " << worldSeed << std::endl;
    m_worldSeed = worldSeed;
    m_worldSeedReceived = true;
    
    // Don't create world/player or change state from this thread!
    // Just set the flag - the main thread will handle the rest
} 