#include "Game.h"
#include "Debug.h"
#include "Block.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <csignal>
#include <thread>
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

Game::Game() : 
    m_window(nullptr),
    m_currentState(GameState::MAIN_MENU),
    m_shouldClose(false),
    m_isHost(false),
    m_myPlayerId(0), // Initialize own player ID
    m_worldSeed(0),
    m_worldSeedReceived(false),
    m_waitingForSpawnChunks(false), // Initialize spawn chunk tracking
    m_gameTime(0.0f),
    m_gameTimeReceived(false),
    m_hasLastSentPosition(false),
    m_firstMouse(true),
    m_lastX(640.0),
    m_lastY(360.0),
    m_deltaTime(0.0f),
    m_lastFrame(0.0f),
    m_fontSmall(nullptr),
    m_fontDefault(nullptr),
    m_fontLarge(nullptr),
    m_fontTitle(nullptr),
    m_showPauseMenu(false),
    m_showInventory(false),
<<<<<<< Updated upstream
    m_showCraftingTable(false),
    m_showFurnace(false),
    m_selectedHotbarSlot(0),
    m_placementPreviewPosition(0, 0, 0),
    m_showPlacementPreview(false)
=======
    m_showUI(true),
    m_selectedHotbarSlot(0)
>>>>>>> Stashed changes
{
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
    
    // Enable antialiasing with 4x MSAA to reduce shimmering
    glfwWindowHint(GLFW_SAMPLES, 4);
    
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
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
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
    
    DEBUG_INFO("Window size: " << windowWidth << "x" << windowHeight);
    DEBUG_INFO("Framebuffer size: " << framebufferWidth << "x" << framebufferHeight);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Load Minecraft font in multiple sizes
    const char* fontPath = "assets/font/mc.otf";
    m_fontSmall = io.Fonts->AddFontFromFileTTF(fontPath, 14.0f);
    m_fontDefault = io.Fonts->AddFontFromFileTTF(fontPath, 16.0f);
    m_fontLarge = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f);
    m_fontTitle = io.Fonts->AddFontFromFileTTF(fontPath, 24.0f);
    
    if (m_fontDefault == nullptr) {
        std::cerr << "Warning: Failed to load Minecraft font from " << fontPath << ", using default font" << std::endl;
    } else {
        DEBUG_INFO("Successfully loaded Minecraft font in multiple sizes!");
        io.FontDefault = m_fontDefault; // Set 16pt as default font
    }

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

    // Initialize item system
    m_itemManager = std::make_unique<ItemManager>();
    if (!m_itemManager->loadFromConfig("items_config.json")) {
        std::cerr << "Warning: Failed to load items config, inventory will be empty" << std::endl;
    }
    
    // Initialize crafting system
    m_craftingSystem = std::make_unique<CraftingSystem>(m_itemManager.get());

    // Player will be created when world becomes available
    
    // Initialize server discovery
    m_serverDiscovery = std::make_unique<ServerDiscovery>();
    if (!m_serverDiscovery->Start()) {
        std::cerr << "Warning: Failed to start server discovery" << std::endl;
        // Don't fail completely, just continue without server discovery
    }

    DEBUG_INFO("Game initialized successfully!");
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
    if (m_currentState == GameState::GAME && m_player && !m_showPauseMenu && !m_showInventory) {
        m_player->ProcessInput(m_window, m_deltaTime, m_world.get(), &(m_renderer.m_blockManager));
    }
}

void Game::SetState(GameState newState) {
    GameState oldState = m_currentState;
    m_currentState = newState;
    std::cout << "State changed to: " << (newState == GameState::MAIN_MENU ? "MAIN_MENU" : "GAME") << std::endl;
    
    // Reset pause menu and inventory when changing states
    m_showPauseMenu = false;
    m_showInventory = false;
    
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
            // Create player at terrain-based spawn position
            Vec3 spawnPos = CalculateSpawnPosition();
            m_player = std::make_unique<Player>(spawnPos.x, spawnPos.y, spawnPos.z);
            
            // Initialize player's inventory with test items
            m_player->InitializeTestInventory(m_itemManager.get());
            
            std::cout << "Created player at terrain spawn position (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")" << std::endl;
            
            // For single player, terrain should be ready immediately, but verify anyway
            if (m_player->VerifyTerrainSafety(m_world.get())) {
                m_player->EnablePhysics();
                std::cout << "Single player terrain verified - physics enabled!" << std::endl;
            } else {
                std::cout << "Single player terrain not safe - keeping physics disabled until verified" << std::endl;
            }
        }
    }
}

void Game::UpdateMainMenu() {
    // Check if we received a world seed and need to create the world
    if (m_worldSeedReceived && !m_world) {
        DEBUG_INFO("Creating world with seed " << m_worldSeed << " in main thread...");
        
        try {
            // Create world with server-provided seed and regenerate with colorful blocks
            m_world = std::make_unique<World>(m_worldSeed);
            
            // Regenerate with colorful blocks using renderer's BlockManager
            // Access BlockManager from renderer to generate colorful world
            m_world->RegenerateWithSeed(m_worldSeed, &(m_renderer.m_blockManager));
            DEBUG_INFO("World created with colorful blocks!");
            
            // DON'T create player immediately - wait for chunks to load first
            // Set up waiting state for multiplayer chunk loading
            if (m_networkClient && m_networkClient->IsConnected()) {
                std::cout << "Requesting initial chunks from server..." << std::endl;
                
                // Request a 3x3 area of chunks around spawn (0,0)
                for (int chunkX = -1; chunkX <= 1; ++chunkX) {
                    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
                        m_networkClient->RequestChunk(chunkX, chunkZ);
                    }
                }
                
                // Initialize spawn chunk tracking
                m_pendingSpawnChunks.clear();
                for (int chunkX = -1; chunkX <= 1; ++chunkX) {
                    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
                        m_pendingSpawnChunks.insert({chunkX, chunkZ});
                    }
                }
                m_waitingForSpawnChunks = true;
                
                std::cout << "Waiting for " << m_pendingSpawnChunks.size() << " spawn chunks to load..." << std::endl;
            } else {
                // Single player mode - create player immediately since world is generated locally
                if (!m_player) {
                    DEBUG_INFO("Creating player at spawn position...");
                    // Create player at terrain-based spawn position
                    Vec3 spawnPos = CalculateSpawnPosition();
                    m_player = std::make_unique<Player>(spawnPos.x, spawnPos.y, spawnPos.z);
                    
                    // Initialize player's inventory with test items
                    m_player->InitializeTestInventory(m_itemManager.get());
                    
                    DEBUG_INFO("Player created at terrain spawn position (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")");
                    
                    // For single player, terrain should be ready immediately, but verify anyway
                    if (m_player->VerifyTerrainSafety(m_world.get())) {
                        m_player->EnablePhysics();
                        std::cout << "Single player terrain verified - physics enabled!" << std::endl;
                    } else {
                        std::cout << "Single player terrain not safe - keeping physics disabled until verified" << std::endl;
                    }
                }
                
                // Immediately change to game state for single player
                DEBUG_INFO("Setting state to GAME...");
                SetState(GameState::GAME);
                DEBUG_INFO("Single player world ready, entering game!");
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR: Failed to create world: " << e.what() << std::endl;
            
            // Reset flags to prevent infinite retry
            m_worldSeedReceived = false;
            m_waitingForSpawnChunks = false;
            m_pendingSpawnChunks.clear();
            
            // Disconnect from server if connection failed during world creation
            if (m_networkClient) {
                std::cout << "Disconnecting due to world creation error..." << std::endl;
                m_networkClient->Disconnect();
                m_networkClient.reset();
            }
            
            // Reset server if we were hosting
            if (m_server && m_isHost) {
                std::cout << "Stopping server due to world creation error..." << std::endl;
                m_server->Stop();
                m_server.reset();
                m_isHost = false;
            }
        }
    }
    
    // Check if we're waiting for spawn chunks and if they're ready
    if (m_waitingForSpawnChunks && m_world && !m_player) {
        // Check if all spawn chunks have been loaded
        bool allChunksLoaded = true;
        for (const auto& chunkCoord : m_pendingSpawnChunks) {
            const Chunk* chunk = m_world->GetChunk(chunkCoord.first, chunkCoord.second);
            if (!chunk) {
                allChunksLoaded = false;
                break;
            }
        }
        
        if (allChunksLoaded) {
            std::cout << "All spawn chunks loaded! Creating player..." << std::endl;
            
            // Create player at terrain-based spawn position
            Vec3 spawnPos = CalculateSpawnPosition();
            m_player = std::make_unique<Player>(spawnPos.x, spawnPos.y, spawnPos.z);
            
            // Initialize player's inventory with test items
            m_player->InitializeTestInventory(m_itemManager.get());
            
            DEBUG_INFO("Player created at terrain spawn position (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")");
            
            // Verify terrain safety before enabling physics
            if (m_player->VerifyTerrainSafety(m_world.get())) {
                m_player->EnablePhysics();
                std::cout << "Terrain verified safe - physics enabled!" << std::endl;
            } else {
                std::cout << "Terrain not yet safe - keeping physics disabled. Will retry in game loop." << std::endl;
            }
            
            // Clear waiting state
            m_waitingForSpawnChunks = false;
            m_pendingSpawnChunks.clear();
            
            // Now it's safe to enter the game
            DEBUG_INFO("Setting state to GAME...");
            SetState(GameState::GAME);
            DEBUG_INFO("World and player ready, entering game!");
        } else {
            // Show loading progress
            int loadedChunks = 0;
            for (const auto& chunkCoord : m_pendingSpawnChunks) {
                const Chunk* chunk = m_world->GetChunk(chunkCoord.first, chunkCoord.second);
                if (chunk) loadedChunks++;
            }
            
            static int lastProgress = -1;
            int currentProgress = (loadedChunks * 100) / m_pendingSpawnChunks.size();
            if (currentProgress != lastProgress) {
                std::cout << "Loading spawn chunks: " << loadedChunks << "/" << m_pendingSpawnChunks.size() 
                         << " (" << currentProgress << "%)" << std::endl;
                lastProgress = currentProgress;
            }
        }
    }
    
    // Main menu logic here
}

void Game::UpdateGame() {
    // Update player physics (gravity, etc.)
    if (m_player && m_world) {
        m_player->Update(m_deltaTime, m_world.get(), &(m_renderer.m_blockManager));
        // Always update FOV interpolation (works in both creative and survival modes)
        m_player->UpdateFOV(m_deltaTime);
    }
    
    // Update first-person arm animation
    m_renderer.UpdateFirstPersonArm(m_deltaTime);
    
    // Update target block for wireframe rendering
    if (m_player && m_world) {
        m_targetBlock = m_player->CastRay(m_world.get(), 5.0f);
    }
    
    // Update block placement preview
    UpdateBlockPlacement();
    
    // Update local game time for smooth progression between server syncs
    if (m_gameTimeReceived) {
        static auto lastTimeUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeUpdate);
        
        // Add elapsed time at normal speed (no acceleration, no wrapping)
        float deltaSeconds = elapsed.count() / 1000.0f;
        m_gameTime += deltaSeconds; // Real-time progression
        
        lastTimeUpdate = now;
    }
    
    // Periodic terrain safety check for players with physics disabled
    if (m_player && m_player->IsSurvivalMode() && !m_player->IsPhysicsEnabled() && m_world) {
        static float lastSafetyCheck = 0.0f;
        static int consecutiveSafeChecks = 0;
        const float safetyCheckInterval = 2.0f; // Check every 2 seconds
        const int requiredConsecutiveChecks = 3; // Require 3 consecutive safe checks before enabling physics
        
        if (glfwGetTime() - lastSafetyCheck > safetyCheckInterval) {
            if (m_player->VerifyTerrainSafety(m_world.get())) {
                consecutiveSafeChecks++;
                std::cout << "[TERRAIN SAFETY] Consecutive safe checks: " << consecutiveSafeChecks << "/" << requiredConsecutiveChecks << std::endl;
                
                if (consecutiveSafeChecks >= requiredConsecutiveChecks) {
                    m_player->EnablePhysics();
                    std::cout << "Terrain safety verified with " << consecutiveSafeChecks << " consecutive checks - physics enabled!" << std::endl;
                    consecutiveSafeChecks = 0; // Reset counter
                }
            } else {
                consecutiveSafeChecks = 0; // Reset if terrain is not safe
                std::cout << "[TERRAIN SAFETY] Terrain not safe - resetting consecutive check counter" << std::endl;
            }
            lastSafetyCheck = glfwGetTime();
        }
    }
    
    // Send player position updates to server
    static float lastPositionSend = 0.0f;
    const float positionSendInterval = 1.0f / 20.0f; // Send 20 times per second
    
    if (glfwGetTime() - lastPositionSend > positionSendInterval) {
        SendPlayerPosition();
        lastPositionSend = glfwGetTime();
    }

    // Process pending block breaks from network
    {
        std::lock_guard<std::mutex> lock(m_pendingBlockBreaksMutex);
        while (!m_pendingBlockBreaks.empty()) {
            auto& breakInfo = m_pendingBlockBreaks.front();
            uint32_t playerId = breakInfo.playerId;
            int32_t x = breakInfo.x;
            int32_t y = breakInfo.y;
            int32_t z = breakInfo.z;

            // Apply block break to client world (if we have one)
            if (m_world) {
                // Use the unified mesh update method for consistency
                m_world->SetBlockWithMeshUpdate(x, y, z, BlockType::AIR, &(m_renderer.m_blockManager));
            }
            m_pendingBlockBreaks.pop();
        }
    }
    
    // Process pending block updates from network
    {
        std::lock_guard<std::mutex> lock(m_pendingBlockUpdatesMutex);
        while (!m_pendingBlockUpdates.empty()) {
            const PendingBlockUpdate& update = m_pendingBlockUpdates.front();
            
            // Apply block update to client world (safe to call OpenGL from main thread)
            if (m_world) {
                try {
                    // Use efficient mesh update method
                    m_world->SetBlockWithMeshUpdate(update.x, update.y, update.z, static_cast<BlockType>(update.blockType), &(m_renderer.m_blockManager));
                } catch (const std::exception& e) {
                    std::cerr << "[CLIENT] Error processing block update: " << e.what() << std::endl;
                }
            }
            
            m_pendingBlockUpdates.pop();
        }
    }
    
    // Process pending chunk data from network
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkDataMutex);
        while (!m_pendingChunkData.empty()) {
            auto& chunkInfo = m_pendingChunkData.front();
            int32_t chunkX = chunkInfo.chunkX;
            int32_t chunkZ = chunkInfo.chunkZ;
            const std::vector<uint16_t>& blockData = chunkInfo.blockData;
            
            std::cout << "[CLIENT] Applying chunk data for (" << chunkX << ", " << chunkZ << ")" << std::endl;
            
            // Apply chunk data to client world (if we have one)
            if (m_world) {
                // Get or create the chunk
                Chunk* chunk = m_world->GetChunk(chunkX, chunkZ);
                if (chunk) {
                    // Apply server data to the chunk
                    chunk->ApplyServerData(blockData.data());
                    
                    // Generate mesh for the updated chunk
                    chunk->GenerateMesh(m_world.get(), &(m_renderer.m_blockManager));
                    
                    std::cout << "[CLIENT] Updated chunk (" << chunkX << ", " << chunkZ << ") with server data" << std::endl;
                    
                    // If we're waiting for spawn chunks, remove this chunk from the pending set
                    if (m_waitingForSpawnChunks) {
                        auto chunkPair = std::make_pair(chunkX, chunkZ);
                        if (m_pendingSpawnChunks.find(chunkPair) != m_pendingSpawnChunks.end()) {
                            m_pendingSpawnChunks.erase(chunkPair);
                            std::cout << "[CLIENT] Spawn chunk (" << chunkX << ", " << chunkZ << ") loaded. " 
                                     << m_pendingSpawnChunks.size() << " remaining." << std::endl;
                        }
                    }
                } else {
                    std::cerr << "[CLIENT] Failed to get chunk (" << chunkX << ", " << chunkZ << ")" << std::endl;
                }
            }
            m_pendingChunkData.pop();
        }
    }
}

void Game::RenderMainMenu() {
    // Create a centered window for the main menu with increased size for server list
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Always);
    
    if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        // Use title font for the main heading
        if (m_fontTitle) {
            ImGui::PushFont(m_fontTitle);
            ImGui::Text("Minecraft Clone - Multiplayer");
            ImGui::PopFont();
        } else {
            ImGui::Text("Minecraft Clone - Multiplayer");
        }
        ImGui::Separator();
        
        if (ImGui::Button("Host Game", ImVec2(580, 50))) {
            StartHost();
        }
        
        ImGui::Separator();
        
        // Available Servers Section with large font header
        if (m_fontLarge) {
            ImGui::PushFont(m_fontLarge);
            ImGui::Text("Available Servers");
            ImGui::PopFont();
        } else {
            ImGui::Text("Available Servers:");
        }
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
        if (m_fontLarge) {
            ImGui::PushFont(m_fontLarge);
            ImGui::Text("Manual Server Entry");
            ImGui::PopFont();
        } else {
            ImGui::Text("Manual Server Entry:");
        }
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
        
        // Render sky first (background)
        if (m_gameTimeReceived) {
            m_renderer.RenderSky(m_gameTime);
        } else {
            static bool debugOnce = false;
            if (!debugOnce) {
                std::cout << "[CLIENT] Game time not yet received from server, using default sky" << std::endl;
                debugOnce = true;
            }
        }
        
        m_renderer.RenderWorld(*m_world, m_gameTime);
        
        // Render wireframe around target block
        if (m_targetBlock.hit) {
            m_renderer.RenderBlockWireframe(m_targetBlock.blockPos, *m_world);
        }
        
        // Render placement preview wireframe (green/different color for placement)
        if (m_showPlacementPreview) {
            // For now, use the same wireframe method but at the placement position
            // TODO: Create a separate method for placement wireframe with different color
            m_renderer.RenderBlockWireframe(m_placementPreviewPosition, *m_world);
        }
        
        // Render other players with interpolated positions
        if (m_networkClient && m_networkClient->IsConnected()) {
            auto interpolatedPositionsMap = GetInterpolatedPlayerPositions();
            // Convert map to vector for renderer
            std::vector<PlayerPosition> interpolatedPositions;
            for (const auto& pair : interpolatedPositionsMap) {
                interpolatedPositions.push_back(pair.second);
            }
            static bool debugOnce = false;
            if (!interpolatedPositions.empty() && !debugOnce) {
                std::cout << "[RENDER] Starting to render " << interpolatedPositions.size() << " other players (my ID: " << m_myPlayerId << ")" << std::endl;
                debugOnce = true;
            }
            m_renderer.RenderOtherPlayers(interpolatedPositions);
        }
        
        // Render first-person arm (like in Minecraft)
        if (m_showUI) {
            m_renderer.RenderFirstPersonArm(*m_player);
        }
        
        m_renderer.EndFrame();
    }
    
    // Show pause menu overlay if active
    if (m_showPauseMenu) {
        RenderPauseMenu();
        return; // Don't show game UI when paused
    }
    
    // Show inventory overlay if active
    if (m_showInventory) {
        RenderInventory();
        return; // Don't show game UI when inventory is open
    }
    
    // Show crafting table interface if active
    if (m_showCraftingTable) {
        RenderCraftingTable();
        return; // Don't show game UI when crafting table is open
    }
    
    // Show furnace interface if active
    if (m_showFurnace) {
        RenderFurnace();
        return; // Don't show game UI when furnace is open
    }
    
    // Show game UI
    if (m_showUI) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);
        
        if (ImGui::Begin("Minecraft Clone", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        // Use large font for the header
        if (m_fontLarge) {
            ImGui::PushFont(m_fontLarge);
            ImGui::Text("3D Block Renderer");
            ImGui::PopFont();
        } else {
            ImGui::Text("3D Block Renderer");
        }
        ImGui::Separator();
        
        if (m_player) {
            Vec3 pos = m_player->GetPosition();
            ImGui::Text("Player Position:");
            ImGui::Text("  X: %.1f, Y: %.1f, Z: %.1f", pos.x, pos.y, pos.z);
            ImGui::Text("Yaw: %.1f, Pitch: %.1f", m_player->GetYaw(), m_player->GetPitch());
            
            // Show survival mode and physics status
            if (m_player->IsSurvivalMode()) {
                if (m_player->IsPhysicsEnabled()) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Mode: Survival (Physics Enabled)");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Mode: Survival (SAFE MODE - Physics Disabled)");
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Enhanced terrain verification in progress...");
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "  Spawned 5 blocks above terrain for safety");
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Mode: Creative");
            }
        }
        
        ImGui::Separator();
        
        // Show time information
        if (m_gameTimeReceived) {
            ImGui::Text("Game Time: %.1f seconds", m_gameTime);
            ImGui::Text("Time of Day: %s", IsDay() ? "Day" : "Night");
            
            // Show time progress bar
            float timeProgress = GetTimeOfDay();
            ImGui::ProgressBar(timeProgress, ImVec2(-1, 0), "");
            ImGui::SameLine(0, 5);
            ImGui::Text("Day Cycle");
        } else {
            ImGui::Text("Waiting for time sync...");
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
        }
        ImGui::End();
    }
    
    // Render hotbar at bottom center of screen
    if (m_showUI) {
        RenderHotbar();
    }
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
        // Center the content with title font
        const char* pausedText = "Game Paused";
        if (m_fontTitle) {
            ImGui::PushFont(m_fontTitle);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(pausedText).x) * 0.5f);
            ImGui::Text("%s", pausedText);
            ImGui::PopFont();
        } else {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(pausedText).x) * 0.5f);
            ImGui::Text("%s", pausedText);
        }
        
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

void Game::RenderInventory() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Create a semi-transparent background overlay
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
    if (ImGui::Begin("InventoryBackground", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoInputs)) {
        // Empty window just for background
    }
    ImGui::End();
    ImGui::PopStyleColor();
    
    // Custom inventory GUI dimensions
    const float slotSize = 64.0f;
    const float slotSpacing = 4.0f;
    const float padding = 20.0f;
    const float titleHeight = 40.0f;
    const float sectionSpacing = 30.0f;
    
    // Calculate total window dimensions (including crafting area)
    const float craftingAreaWidth = (2 * slotSize) + slotSpacing + slotSize + (2 * slotSpacing); // 2x2 grid + result slot + spacing
    const float inventoryWidth = std::max((9 * slotSize) + (8 * slotSpacing), craftingAreaWidth) + (2 * padding);
    const float inventoryHeight = titleHeight + (3 * slotSize) + (2 * slotSpacing) + sectionSpacing + 
                                  (2 * slotSize) + slotSpacing + sectionSpacing + // crafting area
                                  slotSize + (2 * padding); // hotbar + padding
    
    // Center the inventory window
    float centerX = (io.DisplaySize.x - inventoryWidth) * 0.5f;
    float centerY = (io.DisplaySize.y - inventoryHeight) * 0.5f;
    
    ImGui::SetNextWindowSize(ImVec2(inventoryWidth, inventoryHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always);
    
    // Style the main inventory window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.2f, 0.95f)); // Dark blue-gray
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.5f, 0.8f)); // Light gray border
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    
    if (ImGui::Begin("Inventory", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse)) {
        
        if (m_player) {
            const auto& inventory = m_player->GetInventory();
            
            // Render title
            ImGui::PushFont(nullptr); // Use default font but make it prominent
            ImGui::SetCursorPos(ImVec2(padding, padding));
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Inventory");
            ImGui::PopFont();
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f); // Add space after title
            
            // Main inventory section (3x9 grid)
            float startY = ImGui::GetCursorPosY();
            
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 9; ++col) {
                    int slotIndex = row * 9 + col;
                    const auto& slot = inventory.getSlot(slotIndex);
                    
                    float slotX = padding + (col * (slotSize + slotSpacing));
                    float slotY = startY + (row * (slotSize + slotSpacing));
                    
                    RenderCustomInventorySlot(slot, slotX, slotY, slotSize, slotIndex);
                }
            }
            
            // Add crafting area between main inventory and hotbar
            float craftingY = startY + (3 * (slotSize + slotSpacing)) + sectionSpacing;
            
            // Crafting label
            ImGui::SetCursorPos(ImVec2(padding, craftingY - 20.0f));
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.9f, 1.0f), "Crafting");
            
            // Render 2x2 crafting grid
            for (int row = 0; row < 2; ++row) {
                for (int col = 0; col < 2; ++col) {
                    int slotIndex = Inventory::CRAFTING_GRID_START + (row * 2) + col;
                    const auto& slot = inventory.getSlot(slotIndex);
                    
                    float slotX = padding + (col * (slotSize + slotSpacing));
                    float slotY = craftingY + (row * (slotSize + slotSpacing));
                    
                    RenderCustomInventorySlot(slot, slotX, slotY, slotSize, slotIndex);
                }
            }
            
            // Render crafting result slot (to the right of the 2x2 grid)
            const auto& resultSlot = inventory.getSlot(Inventory::CRAFTING_RESULT_SLOT);
            float resultX = padding + (2 * (slotSize + slotSpacing)) + slotSpacing;
            float resultY = craftingY + (slotSize / 2); // Center vertically relative to 2x2 grid
            
            RenderCustomInventorySlot(resultSlot, resultX, resultY, slotSize, Inventory::CRAFTING_RESULT_SLOT);
            
            // Update crafting result based on crafting grid contents
            UpdateCraftingResult();
            
            // Add spacing between crafting area and hotbar
            float hotbarY = craftingY + (2 * (slotSize + slotSpacing)) + sectionSpacing;
            
            // Hotbar label
            ImGui::SetCursorPos(ImVec2(padding, hotbarY - 20.0f));
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.9f, 1.0f), "Hotbar");
            
            // Render hotbar slots (1x9 grid)
            for (int i = 0; i < 9; ++i) {
                const auto& slot = inventory.getHotbarSlot(i);
                
                float slotX = padding + (i * (slotSize + slotSpacing));
                
                RenderCustomInventorySlot(slot, slotX, hotbarY, slotSize, Inventory::HOTBAR_START + i);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(4); // Pop all color styles
    ImGui::PopStyleVar(3); // Pop all var styles
    
    // Render cursor item on top of everything
    RenderCursorItem();
}

void Game::UpdateCraftingResult() {
    if (!m_player || !m_craftingSystem) return;
    
    auto& inventory = m_player->GetInventory();
    
    // Get crafting grid contents
    CraftingRecipe::CraftingSlot craftingSlots[4];
    for (int i = 0; i < 4; i++) {
        const auto& slot = inventory.getCraftingSlot(i);
        if (!slot.isEmpty()) {
            craftingSlots[i] = CraftingRecipe::CraftingSlot(slot.item, slot.quantity);
        }
    }
    
    // Check if we can craft something
    auto result = m_craftingSystem->checkCrafting(craftingSlots);
    
    // Update result slot
    auto& resultSlot = inventory.getCraftingResultSlot();
    if (result.canCraft) {
        resultSlot.item = result.resultItem;
        resultSlot.quantity = result.resultQuantity;
    } else {
        resultSlot.clear();
    }
}

void Game::HandleCraftingResultClick() {
    if (!m_player || !m_craftingSystem) return;
    
    auto& inventory = m_player->GetInventory();
    auto& resultSlot = inventory.getCraftingResultSlot();
    auto& cursorSlot = inventory.getCursorSlot();
    
    if (resultSlot.isEmpty() || !cursorSlot.isEmpty()) {
        return; // No result to take or cursor is already holding something
    }
    
    // Get crafting grid contents
    CraftingRecipe::CraftingSlot craftingSlots[4];
    for (int i = 0; i < 4; i++) {
        auto& slot = inventory.getCraftingSlot(i);
        if (!slot.isEmpty()) {
            craftingSlots[i] = CraftingRecipe::CraftingSlot(slot.item, slot.quantity);
        }
    }
    
    // Perform the crafting (this will consume ingredients)
    auto result = m_craftingSystem->performCrafting(craftingSlots);
    
    if (result.canCraft) {
        // Move result to cursor
        cursorSlot.item = result.resultItem;
        cursorSlot.quantity = result.resultQuantity;
        
        // Update the crafting grid with consumed ingredients
        for (int i = 0; i < 4; i++) {
            auto& slot = inventory.getCraftingSlot(i);
            if (craftingSlots[i].quantity <= 0) {
                slot.clear();
            } else {
                slot.quantity = craftingSlots[i].quantity;
            }
        }
        
        // Clear result slot since we took the item
        resultSlot.clear();
        
        std::cout << "Crafted: " << result.resultQuantity << "x " << result.resultItem->itemName << std::endl;
    }
}

void Game::RenderCustomInventorySlot(const InventorySlot& slot, float x, float y, float size, int slotIndex) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetWindowPos();
    
    // Slot position in screen coordinates
    ImVec2 slot_min(canvas_pos.x + x, canvas_pos.y + y);
    ImVec2 slot_max(slot_min.x + size, slot_min.y + size);
    
    // Check if mouse is hovering over this slot
    bool is_hovered = ImGui::IsMouseHoveringRect(slot_min, slot_max);
    
    // Slot background color
    ImU32 slot_bg_color;
    if (slot.isEmpty()) {
        slot_bg_color = is_hovered ? IM_COL32(70, 70, 80, 255) : IM_COL32(50, 50, 60, 255);
    } else {
        slot_bg_color = is_hovered ? IM_COL32(80, 80, 90, 255) : IM_COL32(60, 60, 70, 255);
    }
    
    // Draw slot background
    draw_list->AddRectFilled(slot_min, slot_max, slot_bg_color, 4.0f);
    
    // Draw slot border
    ImU32 border_color = is_hovered ? IM_COL32(120, 120, 140, 255) : IM_COL32(80, 80, 100, 255);
    draw_list->AddRect(slot_min, slot_max, border_color, 4.0f, 0, 2.0f);
    
    // Render item if slot is not empty
    if (!slot.isEmpty() && slot.item) {
        // Load item texture
        unsigned int itemTexture = m_renderer.GetItemTexture(slot.item->icon);
        
        if (itemTexture != 0) {
            // Item icon size and positioning (slightly smaller than slot for padding)
            const float itemPadding = 8.0f;
            const float itemSize = size - (itemPadding * 2);
            ImVec2 item_min(slot_min.x + itemPadding, slot_min.y + itemPadding);
            ImVec2 item_max(item_min.x + itemSize, item_min.y + itemSize);
            
            // Render item icon
            draw_list->AddImage(reinterpret_cast<void*>(static_cast<uintptr_t>(itemTexture)),
                               item_min, item_max);
            
            // Render quantity text if more than 1
            if (slot.quantity > 1) {
                char quantityText[16];
                snprintf(quantityText, sizeof(quantityText), "%d", slot.quantity);
                
                // Position text in bottom-right corner of slot
                ImVec2 text_size = ImGui::CalcTextSize(quantityText);
                ImVec2 text_pos(slot_max.x - text_size.x - 4.0f, slot_max.y - text_size.y - 4.0f);
                
                // Draw text background for better readability
                ImVec2 text_bg_min(text_pos.x - 2.0f, text_pos.y - 1.0f);
                ImVec2 text_bg_max(text_pos.x + text_size.x + 2.0f, text_pos.y + text_size.y + 1.0f);
                draw_list->AddRectFilled(text_bg_min, text_bg_max, IM_COL32(0, 0, 0, 180), 2.0f);
                
                // Draw quantity text
                draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), quantityText);
            }
        }
    }
    
    // Add hover tooltip with item information
    if (is_hovered && !slot.isEmpty() && slot.item) {
        ImGui::SetTooltip("%s\nQuantity: %d", slot.item->itemName.c_str(), slot.quantity);
    }
    
    // Handle click interactions
    if (is_hovered && ImGui::IsMouseClicked(0)) {
        HandleSlotClick(slotIndex);
    }
}

void Game::RenderCustomHotbarSlot(const InventorySlot& slot, float x, float y, float size, int slotIndex, bool isSelected) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetWindowPos();
    
    // Slot position in screen coordinates
    ImVec2 slot_min(canvas_pos.x + x, canvas_pos.y + y);
    ImVec2 slot_max(slot_min.x + size, slot_min.y + size);
    
    // Check if mouse is hovering over this slot
    bool is_hovered = ImGui::IsMouseHoveringRect(slot_min, slot_max);
    
    // Slot background color - different for selected slot
    ImU32 slot_bg_color;
    if (isSelected) {
        // Selected slot has a brighter background
        slot_bg_color = is_hovered ? IM_COL32(100, 140, 180, 255) : IM_COL32(80, 120, 160, 255);
    } else if (slot.isEmpty()) {
        slot_bg_color = is_hovered ? IM_COL32(70, 70, 80, 255) : IM_COL32(50, 50, 60, 255);
    } else {
        slot_bg_color = is_hovered ? IM_COL32(80, 80, 90, 255) : IM_COL32(60, 60, 70, 255);
    }
    
    // Draw slot background
    draw_list->AddRectFilled(slot_min, slot_max, slot_bg_color, 4.0f);
    
    // Draw slot border - selected slot has a special border
    ImU32 border_color;
    float border_thickness;
    if (isSelected) {
        border_color = IM_COL32(150, 200, 255, 255); // Bright blue for selected
        border_thickness = 3.0f;
    } else {
        border_color = is_hovered ? IM_COL32(120, 120, 140, 255) : IM_COL32(80, 80, 100, 255);
        border_thickness = 2.0f;
    }
    draw_list->AddRect(slot_min, slot_max, border_color, 4.0f, 0, border_thickness);
    
    // Render item if slot is not empty
    if (!slot.isEmpty() && slot.item) {
        // Load item texture
        unsigned int itemTexture = m_renderer.GetItemTexture(slot.item->icon);
        
        if (itemTexture != 0) {
            // Item icon size and positioning (slightly smaller than slot for padding)
            const float itemPadding = 6.0f;
            const float itemSize = size - (itemPadding * 2);
            ImVec2 item_min(slot_min.x + itemPadding, slot_min.y + itemPadding);
            ImVec2 item_max(item_min.x + itemSize, item_min.y + itemSize);
            
            // Render item icon
            draw_list->AddImage(reinterpret_cast<void*>(static_cast<uintptr_t>(itemTexture)),
                               item_min, item_max);
            
            // Render quantity text if more than 1
            if (slot.quantity > 1) {
                char quantityText[16];
                snprintf(quantityText, sizeof(quantityText), "%d", slot.quantity);
                
                // Position text in bottom-right corner of slot
                ImVec2 text_size = ImGui::CalcTextSize(quantityText);
                ImVec2 text_pos(slot_max.x - text_size.x - 3.0f, slot_max.y - text_size.y - 3.0f);
                
                // Draw text background for better readability
                ImVec2 text_bg_min(text_pos.x - 2.0f, text_pos.y - 1.0f);
                ImVec2 text_bg_max(text_pos.x + text_size.x + 2.0f, text_pos.y + text_size.y + 1.0f);
                draw_list->AddRectFilled(text_bg_min, text_bg_max, IM_COL32(0, 0, 0, 200), 2.0f);
                
                // Draw quantity text
                draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), quantityText);
            }
        }
    }
    
    // Add slot number indicator for hotbar
    char slotNumberText[8];
    snprintf(slotNumberText, sizeof(slotNumberText), "%d", (slotIndex - Inventory::HOTBAR_START) + 1);
    ImVec2 number_size = ImGui::CalcTextSize(slotNumberText);
    ImVec2 number_pos(slot_min.x + 2.0f, slot_min.y + 2.0f);
    
    // Draw number background
    ImVec2 number_bg_min(number_pos.x - 1.0f, number_pos.y);
    ImVec2 number_bg_max(number_pos.x + number_size.x + 1.0f, number_pos.y + number_size.y);
    draw_list->AddRectFilled(number_bg_min, number_bg_max, IM_COL32(0, 0, 0, 150), 2.0f);
    
    // Draw slot number
    ImU32 number_color = isSelected ? IM_COL32(255, 255, 0, 255) : IM_COL32(200, 200, 200, 255);
    draw_list->AddText(number_pos, number_color, slotNumberText);
    
    // Add hover tooltip with item information
    if (is_hovered && !slot.isEmpty() && slot.item) {
        ImGui::SetTooltip("Slot %d: %s\nQuantity: %d", (slotIndex - Inventory::HOTBAR_START) + 1, slot.item->itemName.c_str(), slot.quantity);
    }
    
    // Handle click interactions
    if (is_hovered && ImGui::IsMouseClicked(0)) {
        // Update selected hotbar slot when clicking hotbar
        m_selectedHotbarSlot = slotIndex - Inventory::HOTBAR_START;
        
        // Handle item pickup/placement
        HandleSlotClick(slotIndex);
    }
}

void Game::HandleSlotClick(int slotIndex) {
    if (!m_player) return;
    
    auto& inventory = m_player->GetInventory();
    auto& clickedSlot = inventory.getSlot(slotIndex);
    auto& cursorSlot = inventory.getCursorSlot();
    
    // Special handling for crafting result slot
    if (slotIndex == Inventory::CRAFTING_RESULT_SLOT) {
        if (!clickedSlot.isEmpty() && cursorSlot.isEmpty()) {
            // Pick up crafted item and consume ingredients
            HandleCraftingResultClick();
            return;
        } else {
            // Can't place items into result slot
            return;
        }
    }
    
    // If cursor is empty, pick up the clicked item
    if (cursorSlot.isEmpty()) {
        if (!clickedSlot.isEmpty()) {
            // Move item from clicked slot to cursor
            cursorSlot = clickedSlot;
            clickedSlot.clear();
            std::cout << "Picked up: " << cursorSlot.item->itemName << " x" << cursorSlot.quantity << std::endl;
        }
    }
    // If cursor has an item, try to place it
    else {
        if (clickedSlot.isEmpty()) {
            // Place cursor item in empty slot
            clickedSlot = cursorSlot;
            cursorSlot.clear();
            std::cout << "Placed: " << clickedSlot.item->itemName << " x" << clickedSlot.quantity << std::endl;
        }
        else if (clickedSlot.item->itemId == cursorSlot.item->itemId && clickedSlot.item->stackable) {
            // Try to stack items
            int spaceAvailable = clickedSlot.item->maxStackSize - clickedSlot.quantity;
            int amountToAdd = std::min(cursorSlot.quantity, spaceAvailable);
            
            if (amountToAdd > 0) {
                clickedSlot.quantity += amountToAdd;
                cursorSlot.quantity -= amountToAdd;
                
                if (cursorSlot.quantity <= 0) {
                    cursorSlot.clear();
                }
                
                std::cout << "Stacked items: " << clickedSlot.item->itemName << " x" << clickedSlot.quantity << std::endl;
            }
            else {
                // Can't stack, swap items instead
                SwapSlots(slotIndex, Inventory::CURSOR_SLOT);
            }
        }
        else {
            // Different items, swap them
            SwapSlots(slotIndex, Inventory::CURSOR_SLOT);
            std::cout << "Swapped items" << std::endl;
        }
    }
}

void Game::SwapSlots(int slotA, int slotB) {
    if (!m_player) return;
    
    auto& inventory = m_player->GetInventory();
    auto& slotAData = inventory.getSlot(slotA);
    auto& slotBData = inventory.getSlot(slotB);
    
    // Simple swap
    InventorySlot temp = slotAData;
    slotAData = slotBData;
    slotBData = temp;
}

void Game::RenderCursorItem() {
    if (!m_player) return;
    
    const auto& inventory = m_player->GetInventory();
    const auto& cursorSlot = inventory.getCursorSlot();
    
    // Only render if cursor slot has an item
    if (cursorSlot.isEmpty() || !cursorSlot.item) return;
    
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    
    // Get mouse position
    ImVec2 mouse_pos = io.MousePos;
    
    // Item size for cursor (slightly smaller than slot size)
    const float itemSize = 32.0f;
    const float halfSize = itemSize * 0.5f;
    
    // Position item centered on mouse cursor
    ImVec2 item_min(mouse_pos.x - halfSize, mouse_pos.y - halfSize);
    ImVec2 item_max(mouse_pos.x + halfSize, mouse_pos.y + halfSize);
    
    // Load and render item texture
    unsigned int itemTexture = m_renderer.GetItemTexture(cursorSlot.item->icon);
    if (itemTexture != 0) {
        // Add semi-transparent background for better visibility
        ImVec2 bg_min(item_min.x - 2, item_min.y - 2);
        ImVec2 bg_max(item_max.x + 2, item_max.y + 2);
        draw_list->AddRectFilled(bg_min, bg_max, IM_COL32(0, 0, 0, 100), 4.0f);
        
        // Render item icon with slight transparency to show it's being dragged
        draw_list->AddImage(reinterpret_cast<void*>(static_cast<uintptr_t>(itemTexture)),
                           item_min, item_max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 200));
        
        // Render quantity text if more than 1
        if (cursorSlot.quantity > 1) {
            char quantityText[16];
            snprintf(quantityText, sizeof(quantityText), "%d", cursorSlot.quantity);
            
            // Position text in bottom-right corner
            ImVec2 text_size = ImGui::CalcTextSize(quantityText);
            ImVec2 text_pos(item_max.x - text_size.x - 2.0f, item_max.y - text_size.y - 2.0f);
            
            // Draw text background
            ImVec2 text_bg_min(text_pos.x - 2.0f, text_pos.y - 1.0f);
            ImVec2 text_bg_max(text_pos.x + text_size.x + 2.0f, text_pos.y + text_size.y + 1.0f);
            draw_list->AddRectFilled(text_bg_min, text_bg_max, IM_COL32(0, 0, 0, 200), 2.0f);
            
            // Draw quantity text
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), quantityText);
        }
    }
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
                // Close crafting interfaces first if open
                if (s_instance->m_showCraftingTable || s_instance->m_showFurnace) {
                    s_instance->m_showCraftingTable = false;
                    s_instance->m_showFurnace = false;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else {
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
        }
        
        // Toggle survival mode with backslash key
        if (key == GLFW_KEY_BACKSLASH && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME && s_instance->m_player && s_instance->m_world) {
                s_instance->m_player->ToggleSurvivalMode(s_instance->m_world.get());
                std::cout << "Survival mode: " << (s_instance->m_player->IsSurvivalMode() ? "ON" : "OFF") << std::endl;
            }
        }
        
        // Toggle frustum culling with F1 key for debugging
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME) {
                s_instance->m_renderer.m_enableFrustumCulling = !s_instance->m_renderer.m_enableFrustumCulling;
                std::cout << "Frustum culling: " << (s_instance->m_renderer.m_enableFrustumCulling ? "ON" : "OFF") << std::endl;
            }
        }
        
        // Toggle UI visibility with ] key
        if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME) {
                s_instance->m_showUI = !s_instance->m_showUI;
                std::cout << "UI visibility: " << (s_instance->m_showUI ? "ON" : "OFF") << std::endl;
            }
        }
        
        // Toggle inventory with E key
        if (key == GLFW_KEY_E && action == GLFW_PRESS) {
            if (s_instance->m_currentState == GameState::GAME && !s_instance->m_showPauseMenu) {
                s_instance->m_showInventory = !s_instance->m_showInventory;
                
                // If opening inventory, close other interfaces
                if (s_instance->m_showInventory) {
                    s_instance->m_showCraftingTable = false;
                    s_instance->m_showFurnace = false;
                }
                
                // Update cursor visibility based on inventory state
                if (s_instance->m_showInventory) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
        }
        
        // Handle number keys (1-9) for hotbar selection
        if (s_instance->m_currentState == GameState::GAME && action == GLFW_PRESS) {
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
                int hotbarSlot = key - GLFW_KEY_1; // Convert to 0-8 range
                s_instance->m_selectedHotbarSlot = hotbarSlot;
                std::cout << "Selected hotbar slot: " << (hotbarSlot + 1) << std::endl;
            }
        }
    }
}

void Game::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance && s_instance->m_currentState == GameState::GAME && s_instance->m_player && !s_instance->m_showPauseMenu && !s_instance->m_showInventory && !s_instance->m_showCraftingTable && !s_instance->m_showFurnace) {
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

void Game::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance && s_instance->m_currentState == GameState::GAME && s_instance->m_player && s_instance->m_world && !s_instance->m_showPauseMenu && !s_instance->m_showInventory) {
        // Trigger punch animation for left or right click
        if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
            s_instance->m_renderer.TriggerArmPunch();
        }
        
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            // Cast ray to find target block
            RaycastResult raycast = s_instance->m_player->CastRay(s_instance->m_world.get(), 5.0f);
            
            if (raycast.hit) {
                int blockX = (int)raycast.blockPos.x;
                int blockY = (int)raycast.blockPos.y;
                int blockZ = (int)raycast.blockPos.z;
                
                // Get the block type before breaking it
                Block targetBlock = s_instance->m_world->GetBlock(blockX, blockY, blockZ);
                BlockType blockType = targetBlock.GetType();
                
                // DEBUG: Log what block type we're breaking
                std::cout << "Breaking block type: " << static_cast<int>(blockType) << std::endl;
                
                // Don't collect AIR blocks
                if (blockType != BlockType::AIR) {
                    // Get the corresponding item for this block
                    Item* blockItem = s_instance->m_itemManager->getItemForBlock(blockType);
                    if (blockItem) {
                        // Add the item to player's inventory
                        if (s_instance->m_player->GetInventory().addItem(blockItem, 1)) {
                            std::cout << "Added " << blockItem->itemName << " to inventory (block type " << static_cast<int>(blockType) << ")" << std::endl;
                        } else {
                            std::cout << "Inventory full! Could not add " << blockItem->itemName << std::endl;
                        }
                    } else {
                        std::cout << "No item found for block type " << static_cast<int>(blockType) << std::endl;
                    }
                }
                
                std::cout << "Breaking block at (" << blockX << ", " << blockY << ", " << blockZ << ")" << std::endl;
                
                // Break the block immediately on client (client prediction) with efficient mesh update
                s_instance->m_world->SetBlockWithMeshUpdate(blockX, blockY, blockZ, BlockType::AIR, &(s_instance->m_renderer.m_blockManager));
                
                // Send to server for synchronization with other clients using the new streaming system
                if (s_instance->m_networkClient && s_instance->m_networkClient->IsConnected()) {
                    s_instance->m_networkClient->SendBlockUpdate(blockX, blockY, blockZ, static_cast<uint16_t>(BlockType::AIR));
                }
            }
        }
        
        // Right-click handling for block placement and interactions
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            // First check if we're holding a placeable item for block placement
            if (s_instance->IsHoldingPlaceableItem() && s_instance->m_showPlacementPreview) {
                // Get currently selected hotbar item
                const InventorySlot& selectedSlot = s_instance->m_player->GetInventory().getHotbarSlot(s_instance->m_selectedHotbarSlot);
                
                // Find the item key to get the corresponding block type
                std::string itemKey = "";
                const auto& allItems = s_instance->m_itemManager->getAllItems();
                for (const auto& pair : allItems) {
                    if (pair.second.get() == selectedSlot.item) {
                        itemKey = pair.first;
                        break;
                    }
                }
                
                if (!itemKey.empty()) {
                    BlockType blockTypeToPlace = s_instance->m_itemManager->getBlockTypeForItem(itemKey);
                    if (blockTypeToPlace != BlockType::AIR) {
                        // Place the block at the preview position
                        int placeX = (int)s_instance->m_placementPreviewPosition.x;
                        int placeY = (int)s_instance->m_placementPreviewPosition.y;
                        int placeZ = (int)s_instance->m_placementPreviewPosition.z;
                        
                        std::cout << "Placing " << itemKey << " block at (" << placeX << ", " << placeY << ", " << placeZ << ")" << std::endl;
                        
                        // Place the block in the world with efficient mesh update
                        s_instance->m_world->SetBlockWithMeshUpdate(placeX, placeY, placeZ, blockTypeToPlace, &(s_instance->m_renderer.m_blockManager));
                        
                        // Remove one item from inventory
                        s_instance->m_player->GetInventory().getHotbarSlot(s_instance->m_selectedHotbarSlot).removeItems(1);
                        
                        // Send to server for synchronization
                        if (s_instance->m_networkClient && s_instance->m_networkClient->IsConnected()) {
                            s_instance->m_networkClient->SendBlockUpdate(placeX, placeY, placeZ, static_cast<uint16_t>(blockTypeToPlace));
                        }
                        
                        return; // Don't handle block interactions when placing
                    }
                }
            }
            
            // If not placing blocks, handle block interactions
            RaycastResult raycast = s_instance->m_player->CastRay(s_instance->m_world.get(), 5.0f);
            
            if (raycast.hit) {
                int blockX = (int)raycast.blockPos.x;
                int blockY = (int)raycast.blockPos.y;
                int blockZ = (int)raycast.blockPos.z;
                
                // Get the block type to check for interactable blocks
                Block targetBlock = s_instance->m_world->GetBlock(blockX, blockY, blockZ);
                BlockType blockType = targetBlock.GetType();
                
                std::cout << "Right-clicking block type: " << static_cast<int>(blockType) << std::endl;
                
                // Handle block interactions
                if (blockType == BlockType::CRAFTING_TABLE) {
                    s_instance->m_showCraftingTable = true;
                    s_instance->m_showInventory = false;
                    s_instance->m_showFurnace = false;
                    // Enable cursor for UI interaction
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    std::cout << "Opened crafting table interface" << std::endl;
                } else if (blockType == BlockType::FURNACE) {
                    s_instance->m_showFurnace = true;
                    s_instance->m_showInventory = false;
                    s_instance->m_showCraftingTable = false;
                    // Enable cursor for UI interaction
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    std::cout << "Opened furnace interface" << std::endl;
                }
            }
        }
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
        
        // Give server time to fully initialize before client connects
        DEBUG_INFO("Server started, waiting for initialization...");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Also connect as a client to our own server
        m_networkClient = std::make_unique<NetworkClient>();
        
        // Set up callbacks with error handling
        m_networkClient->SetPlayerJoinCallback([this](uint32_t playerId, const PlayerPosition& position) {
            try {
                OnPlayerJoin(playerId, position);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerJoin: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetPlayerLeaveCallback([this](uint32_t playerId) {
            try {
                OnPlayerLeave(playerId);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerLeave: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetPlayerPositionCallback([this](uint32_t playerId, const PlayerPosition& position) {
            try {
                OnPlayerPositionUpdate(playerId, position);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerPositionUpdate: " << e.what() << std::endl;
            }
        });

        m_networkClient->SetWorldSeedCallback([this](int32_t worldSeed) {
            try {
                OnWorldSeedReceived(worldSeed);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnWorldSeedReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetGameTimeCallback([this](float gameTime) {
            try {
                OnGameTimeReceived(gameTime);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnGameTimeReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetMyPlayerIdCallback([this](uint32_t myPlayerId) {
            try {
                OnMyPlayerIdReceived(myPlayerId);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnMyPlayerIdReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetBlockBreakCallback([this](uint32_t playerId, int32_t x, int32_t y, int32_t z) {
            try {
                OnBlockBreakReceived(playerId, x, y, z);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnBlockBreakReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetBlockUpdateCallback([this](uint32_t playerId, int32_t x, int32_t y, int32_t z, uint16_t blockType) {
            try {
                OnBlockUpdateReceived(playerId, x, y, z, blockType);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnBlockUpdateReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetChunkDataCallback([this](int32_t chunkX, int32_t chunkZ, const uint16_t* blockData) {
            try {
                OnChunkDataReceived(chunkX, chunkZ, blockData);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnChunkDataReceived: " << e.what() << std::endl;
            }
        });
        
        if (m_networkClient->Connect("127.0.0.1", 8080)) {
            // Wait for world seed from server before creating world
            std::cout << "Connected to own server, waiting for world seed..." << std::endl;
        } else {
            std::cerr << "Failed to connect to own server" << std::endl;
            m_server->Stop();
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
    try {
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
        
        // Set up callbacks with error handling
        m_networkClient->SetPlayerJoinCallback([this](uint32_t playerId, const PlayerPosition& position) {
            try {
                OnPlayerJoin(playerId, position);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerJoin: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetPlayerLeaveCallback([this](uint32_t playerId) {
            try {
                OnPlayerLeave(playerId);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerLeave: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetPlayerPositionCallback([this](uint32_t playerId, const PlayerPosition& position) {
            try {
                OnPlayerPositionUpdate(playerId, position);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnPlayerPositionUpdate: " << e.what() << std::endl;
            }
        });

        m_networkClient->SetWorldSeedCallback([this](int32_t worldSeed) {
            try {
                OnWorldSeedReceived(worldSeed);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnWorldSeedReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetGameTimeCallback([this](float gameTime) {
            try {
                OnGameTimeReceived(gameTime);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnGameTimeReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetMyPlayerIdCallback([this](uint32_t myPlayerId) {
            try {
                OnMyPlayerIdReceived(myPlayerId);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnMyPlayerIdReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetBlockBreakCallback([this](uint32_t playerId, int32_t x, int32_t y, int32_t z) {
            try {
                OnBlockBreakReceived(playerId, x, y, z);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnBlockBreakReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetBlockUpdateCallback([this](uint32_t playerId, int32_t x, int32_t y, int32_t z, uint16_t blockType) {
            try {
                OnBlockUpdateReceived(playerId, x, y, z, blockType);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnBlockUpdateReceived: " << e.what() << std::endl;
            }
        });
        
        m_networkClient->SetChunkDataCallback([this](int32_t chunkX, int32_t chunkZ, const uint16_t* blockData) {
            try {
                OnChunkDataReceived(chunkX, chunkZ, blockData);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in OnChunkDataReceived: " << e.what() << std::endl;
            }
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
    } catch (const std::exception& e) {
        std::cerr << "ERROR in JoinServer: " << e.what() << std::endl;
        if (m_networkClient) {
            m_networkClient.reset();
        }
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
        
        // Only send update if position or rotation has changed significantly
        bool shouldSend = false;
        
        if (!m_hasLastSentPosition) {
            // First time sending position
            shouldSend = true;
        } else {
            // Check for position changes
            float positionDelta = sqrtf(
                (playerPos.x - m_lastSentPlayerPosition.x) * (playerPos.x - m_lastSentPlayerPosition.x) +
                (playerPos.y - m_lastSentPlayerPosition.y) * (playerPos.y - m_lastSentPlayerPosition.y) +
                (playerPos.z - m_lastSentPlayerPosition.z) * (playerPos.z - m_lastSentPlayerPosition.z)
            );
            
            // Check for rotation changes
            float yawDelta = fabsf(playerPos.yaw - m_lastSentPlayerPosition.yaw);
            float pitchDelta = fabsf(playerPos.pitch - m_lastSentPlayerPosition.pitch);
            
            // Handle yaw wrap-around (360 degrees)
            if (yawDelta > 180.0f) {
                yawDelta = 360.0f - yawDelta;
            }
            
            if (positionDelta >= POSITION_CHANGE_THRESHOLD || 
                yawDelta >= ROTATION_CHANGE_THRESHOLD ||
                pitchDelta >= ROTATION_CHANGE_THRESHOLD) {
                shouldSend = true;
            }
        }
        
        if (shouldSend) {
            std::cout << "[CLIENT] Sending position (" << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ") yaw=" << playerPos.yaw << std::endl;
            m_networkClient->SendPlayerPosition(playerPos);
            m_lastSentPlayerPosition = playerPos;
            m_hasLastSentPosition = true;
        }
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
    // If this is our own player in multiplayer, update our position to the server's spawn position
    if (playerId == m_myPlayerId && m_networkClient && m_networkClient->IsConnected()) {
        if (m_player) {
            m_player->SetPosition(Vec3(position.x, position.y, position.z));
            m_player->SetRotation(position.yaw, position.pitch);
            std::cout << "[GAME] Updated own player position to server spawn: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
        }
        return; // Don't add ourselves to the other players list
    }
    
    // Create new interpolated player for other players
    InterpolatedPlayer& player = m_otherPlayers[playerId];
    player.currentPos = position;
    player.previousPos = position; // Start with same position to avoid interpolation artifacts
    player.lastUpdateTime = std::chrono::steady_clock::now();
    player.previousUpdateTime = player.lastUpdateTime;
    
    std::cout << "[GAME] Player " << playerId << " joined at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void Game::OnPlayerLeave(uint32_t playerId) {
    m_otherPlayers.erase(playerId);
    std::cout << "Player " << playerId << " left the game" << std::endl;
}

void Game::OnPlayerPositionUpdate(uint32_t playerId, const PlayerPosition& position) {
    // Don't process our own position updates (we already have our own position)
    if (playerId == m_myPlayerId) {
        return;
    }
    
    std::cout << "[GAME] Received position update for player " << playerId << " at (" << position.x << ", " << position.y << ", " << position.z << ") yaw=" << position.yaw << std::endl;
    
    auto it = m_otherPlayers.find(playerId);
    if (it != m_otherPlayers.end()) {
        std::cout << "[GAME] Updating existing player " << playerId << " position" << std::endl;
        it->second.UpdatePosition(position);
    } else {
        std::cout << "[GAME] ERROR: Player " << playerId << " not found in m_otherPlayers map!" << std::endl;
        std::cout << "[GAME] Current m_otherPlayers size: " << m_otherPlayers.size() << std::endl;
        for (const auto& pair : m_otherPlayers) {
            std::cout << "[GAME]   - Player " << pair.first << " in map" << std::endl;
        }
    }
}

void Game::OnWorldSeedReceived(int32_t worldSeed) {
    std::cout << "Received world seed from server: " << worldSeed << std::endl;
    m_worldSeed = worldSeed;
    m_worldSeedReceived = true;
    
    // Don't create world/player or change state from this thread!
    // Just set the flag - the main thread will handle the rest
}

void Game::OnGameTimeReceived(float gameTime) {
    std::cout << "[CLIENT] Time sync - server: " << gameTime << " seconds (" 
              << (gameTime < 450.0f ? "DAY" : "NIGHT") << "), client was: " << m_gameTime << std::endl;
    
    // Update our time and reset the client-side timer
    m_gameTime = gameTime;
    m_gameTimeReceived = true;
    
    // Reset the client-side time update reference point
    static auto* lastTimeUpdate = []() {
        static auto time = std::chrono::steady_clock::now();
        return &time;
    }();
    *lastTimeUpdate = std::chrono::steady_clock::now();
}

void Game::OnMyPlayerIdReceived(uint32_t myPlayerId) {
    std::cout << "Received my player ID from server: " << myPlayerId << std::endl;
    m_myPlayerId = myPlayerId;
}

void Game::OnBlockBreakReceived(uint32_t playerId, int32_t x, int32_t y, int32_t z) {
    std::cout << "[CLIENT] Received block break from player " << playerId << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
    
    // Apply block break to client world (this is from another player or server confirmation)
    if (m_world) {
        try {
            // Use efficient mesh update method
            m_world->SetBlockWithMeshUpdate(x, y, z, BlockType::AIR, &(m_renderer.m_blockManager));
            std::cout << "[CLIENT] Applied block break at (" << x << ", " << y << ", " << z << ") with incremental mesh update" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[CLIENT] Error processing block break: " << e.what() << std::endl;
        }
    } else {
        std::cout << "[CLIENT] Warning: No world available for block break at (" << x << ", " << y << ", " << z << ")" << std::endl;
    }
}

void Game::OnBlockUpdateReceived(uint32_t playerId, int32_t x, int32_t y, int32_t z, uint16_t blockType) {
    // Queue the block update for processing on the main thread (to avoid OpenGL calls from network thread)
    {
        std::lock_guard<std::mutex> lock(m_pendingBlockUpdatesMutex);
        PendingBlockUpdate update;
        update.playerId = playerId;
        update.x = x;
        update.y = y;
        update.z = z;
        update.blockType = blockType;
        m_pendingBlockUpdates.push(update);
    }
}

void Game::OnChunkDataReceived(int32_t chunkX, int32_t chunkZ, const uint16_t* blockData) {
    std::cout << "[CLIENT] Queuing chunk data for (" << chunkX << ", " << chunkZ << ")" << std::endl;
    
    // Queue the chunk data for processing on the main thread
    // (OpenGL operations must happen on the main thread)
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkDataMutex);
        PendingChunkData chunkData;
        chunkData.chunkX = chunkX;
        chunkData.chunkZ = chunkZ;
        // Copy the block data (16x256x16 = 65536 bytes)
        chunkData.blockData.assign(blockData, blockData + (16 * 256 * 16));
        m_pendingChunkData.push(std::move(chunkData));
    }
}

bool Game::IsDay() const {
    return m_gameTime < 450.0f; // First 7.5 minutes (450 seconds) is day
}

bool Game::IsNight() const {
    return m_gameTime >= 450.0f; // Last 7.5 minutes (450-900 seconds) is night
}

float Game::GetTimeOfDay() const {
    // Convert game time (0-900 seconds) to 0.0-1.0 
    // where 0.0 = sunrise, 0.5 = sunset, 1.0 = sunrise again
    return m_gameTime / 900.0f;
}

void Game::RenderHotbar() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Custom hotbar dimensions
    const float slotSize = 48.0f;
    const float slotSpacing = 4.0f;
    const float padding = 8.0f;
    const float margin = 20.0f; // Distance from bottom of screen
    
    // Calculate total hotbar dimensions
    const float hotbarWidth = (9 * slotSize) + (8 * slotSpacing) + (2 * padding);
    const float hotbarHeight = slotSize + (2 * padding);
    
    // Position hotbar at bottom center of screen
    ImVec2 windowPos = ImVec2(
        (io.DisplaySize.x - hotbarWidth) * 0.5f,  // Center horizontally
        io.DisplaySize.y - hotbarHeight - margin  // Bottom with margin
    );
    
    // Create hotbar window
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight), ImGuiCond_Always);
    
    // Style the hotbar window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.9f)); // Dark background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.4f, 0.8f)); // Subtle border
    
    if (ImGui::Begin("Hotbar", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        
        if (m_player) {
            const auto& inventory = m_player->GetInventory();
            
            // Render hotbar slots (1x9 grid)
            for (int i = 0; i < 9; ++i) {
                const auto& slot = inventory.getHotbarSlot(i);
                bool isSelected = (i == m_selectedHotbarSlot);
                
                float slotX = padding + (i * (slotSize + slotSpacing));
                float slotY = padding;
                
                RenderCustomHotbarSlot(slot, slotX, slotY, slotSize, Inventory::HOTBAR_START + i, isSelected);
            }
        }
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2); // Remove background and border colors
    ImGui::PopStyleVar(3); // Remove padding, rounding, and border styles
    
    // Render cursor item on top of hotbar too
    RenderCursorItem();
}


Vec3 Game::CalculateSpawnPosition() const {
    if (!m_world) {
        // No world available, use default safe height
        std::cout << "[SPAWN] No world available, using default height 75" << std::endl;
        return Vec3(0.0f, 75.0f, 0.0f);
    }
    
    // Find highest block at world origin (0, 0)
    int highestY = m_world->FindHighestBlock(0, 0);
    
    // Safety check: if terrain height seems wrong, use a safe fallback
    if (highestY < 10 || highestY > 200) {
        std::cout << "[SPAWN] Detected unusual terrain height " << highestY << ", using safe fallback" << std::endl;
        return Vec3(0.0f, 80.0f, 0.0f); // Safe height above typical terrain
    }
    
    // Spawn player 5 blocks above the highest block for extra safety in survival mode
    float spawnY = static_cast<float>(highestY + 5);
    std::cout << "[SPAWN] Calculated spawn position at (0, " << spawnY << ", 0) based on terrain height " << highestY << " (5-block safety buffer)" << std::endl;
    
    return Vec3(0.0f, spawnY, 0.0f);
}

void Game::RenderCraftingTable() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Create a semi-transparent background overlay
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
    if (ImGui::Begin("CraftingTableBackground", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoInputs)) {
        // Empty window just for background
    }
    ImGui::End();
    ImGui::PopStyleColor();
    
    // GUI dimensions
    const float slotSize = 64.0f;
    const float slotSpacing = 4.0f;
    const float padding = 20.0f;
    const float titleHeight = 40.0f;
    const float sectionSpacing = 20.0f;
    
    // Calculate window dimensions (3x3 crafting grid + result slot + player inventory)
    const float craftingGridWidth = (3 * slotSize) + (2 * slotSpacing);
    const float craftingAreaWidth = craftingGridWidth + slotSize + (3 * slotSpacing); // grid + result + spacing
    const float inventoryWidth = std::max((9 * slotSize) + (8 * slotSpacing), craftingAreaWidth) + (2 * padding);
    const float craftingHeight = (3 * slotSize) + (2 * slotSpacing);
    const float inventoryHeight = titleHeight + craftingHeight + sectionSpacing + 
                                 (3 * slotSize) + (2 * slotSpacing) + sectionSpacing + // main inventory
                                 slotSize + (2 * padding); // hotbar + padding
    
    // Center the window
    float centerX = (io.DisplaySize.x - inventoryWidth) * 0.5f;
    float centerY = (io.DisplaySize.y - inventoryHeight) * 0.5f;
    
    ImGui::SetNextWindowSize(ImVec2(inventoryWidth, inventoryHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always);
    
    // Style the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.2f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    
    if (ImGui::Begin("Crafting Table", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse)) {
        
        if (m_player) {
            // Crafting area
            ImGui::Text("Crafting");
            ImGui::Separator();
            
            // 3x3 crafting grid (placeholder for now)
            for (int row = 0; row < 3; row++) {
                if (row > 0) ImGui::SameLine();
                for (int col = 0; col < 3; col++) {
                    if (col > 0) ImGui::SameLine();
                    
                    // Create unique ID for each slot
                    std::string slotId = "craft_" + std::to_string(row * 3 + col);
                    ImGui::PushID(slotId.c_str());
                    
                    // Draw empty crafting slot
                    ImVec2 cursorPos = ImGui::GetCursorPos();
                    ImGui::Button("", ImVec2(slotSize, slotSize));
                    
                    ImGui::PopID();
                }
                ImGui::NewLine();
            }
            
            // Result slot (to the right of crafting grid)
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + slotSpacing * 2);
            ImGui::Button("Result", ImVec2(slotSize, slotSize));
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Player inventory
            ImGui::Text("Inventory");
            const Inventory& inventory = m_player->GetInventory();
            
            // Main inventory (3 rows of 9 slots each)
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 9; col++) {
                    if (col > 0) ImGui::SameLine(0, slotSpacing);
                    
                    int slotIndex = row * 9 + col;
                    const InventorySlot& slot = inventory.getSlot(slotIndex);
                    RenderCustomInventorySlot(slot, 0, 0, slotSize, slotIndex);
                }
                if (row < 2) ImGui::Spacing();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Hotbar
            ImGui::Text("Hotbar");
            for (int i = 0; i < 9; i++) {
                if (i > 0) ImGui::SameLine(0, slotSpacing);
                
                int slotIndex = 27 + i; // Hotbar starts at slot 27
                const InventorySlot& slot = inventory.getSlot(slotIndex);
                bool isSelected = (i == m_selectedHotbarSlot);
                RenderCustomHotbarSlot(slot, 0, 0, slotSize, slotIndex, isSelected);
            }
        }
    }
    ImGui::End();
    
    // Pop all style modifications
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

// Block placement system implementation
void Game::UpdateBlockPlacement() {
    if (!m_player || !m_world || !m_itemManager) {
        m_showPlacementPreview = false;
        return;
    }
    
    // Check if player is holding a placeable item
    if (!IsHoldingPlaceableItem()) {
        m_showPlacementPreview = false;
        return;
    }
    
    // Cast ray to find placement position
    RaycastResult raycast = m_player->CastRay(m_world.get(), 5.0f);
    if (!raycast.hit) {
        m_showPlacementPreview = false;
        return;
    }
    
    // Calculate placement position (adjacent to hit surface)
    Vec3 placementPos = GetBlockPlacementPosition(raycast);
    
    // Check if block can be placed at this position
    if (!CanPlaceBlock(placementPos)) {
        m_showPlacementPreview = false;
        return;
    }
    
    // Show placement preview
    m_placementPreviewPosition = placementPos;
    m_showPlacementPreview = true;
}

Vec3 Game::GetBlockPlacementPosition(const RaycastResult& raycast) const {
    // Place block adjacent to the hit surface using the normal
    Vec3 placementPos;
    placementPos.x = raycast.blockPos.x + raycast.normal.x;
    placementPos.y = raycast.blockPos.y + raycast.normal.y;
    placementPos.z = raycast.blockPos.z + raycast.normal.z;
    return placementPos;
}

bool Game::CanPlaceBlock(const Vec3& position) const {
    if (!m_world) return false;
    
    // Check if the position is already occupied by a solid block
    Block existingBlock = m_world->GetBlock((int)position.x, (int)position.y, (int)position.z);
    if (existingBlock.IsSolid()) {
        return false; // Can't place block where there's already a solid block
    }
    
    // Check if player would intersect with the placed block
    if (m_player) {
        Vec3 playerPos = m_player->GetPosition();
        float playerHeight = m_player->GetPlayerHeight();
        float playerWidth = m_player->GetPlayerWidth();
        
        // Check if player's bounding box intersects with the block to be placed
        // Player extends from playerPos.y to playerPos.y + playerHeight
        // Player width extends from playerPos +/- playerWidth/2 in X and Z
        
        if (position.x >= playerPos.x - playerWidth/2 && position.x < playerPos.x + playerWidth/2 &&
            position.z >= playerPos.z - playerWidth/2 && position.z < playerPos.z + playerWidth/2 &&
            position.y >= playerPos.y && position.y < playerPos.y + playerHeight) {
            return false; // Would intersect with player
        }
    }
    
    return true;
}

bool Game::IsHoldingPlaceableItem() const {
    if (!m_player || !m_itemManager) return false;
    
    // Get currently selected hotbar item
    const InventorySlot& selectedSlot = m_player->GetInventory().getHotbarSlot(m_selectedHotbarSlot);
    if (selectedSlot.isEmpty() || !selectedSlot.item) {
        return false;
    }
    
    // Check if the item has a corresponding block type (can be placed)
    // We try to convert the item to a block type - if it returns AIR, it's not placeable
    std::string itemKey = ""; // We need to get the item key from the item
    
    // Find the item key by searching the ItemManager's items
    const auto& allItems = m_itemManager->getAllItems();
    for (const auto& pair : allItems) {
        if (pair.second.get() == selectedSlot.item) {
            itemKey = pair.first;
            break;
        }
    }
    
    if (itemKey.empty()) return false;
    
    BlockType blockType = m_itemManager->getBlockTypeForItem(itemKey);
    return blockType != BlockType::AIR;
}

void Game::RenderFurnace() {
    // Placeholder furnace interface - similar to crafting table but simpler
    ImGuiIO& io = ImGui::GetIO();
    
    // Style constants
    const float padding = 20.0f;
    const float slotSize = 64.0f;
    const float slotSpacing = 8.0f;
    const float titleHeight = 40.0f;
    const float sectionSpacing = 20.0f;
    
    // Calculate window dimensions for furnace interface
    const float furnaceWidth = (3 * slotSize) + (2 * slotSpacing) + (2 * padding); // 3 slots: input, fuel, output
    const float inventoryWidth = std::max((9 * slotSize) + (8 * slotSpacing), furnaceWidth) + (2 * padding);
    const float furnaceHeight = titleHeight + slotSize + sectionSpacing + 
                               (3 * slotSize) + (2 * slotSpacing) + sectionSpacing + // main inventory
                               slotSize + (2 * padding); // hotbar + padding
    
    // Center the window
    float centerX = (io.DisplaySize.x - inventoryWidth) * 0.5f;
    float centerY = (io.DisplaySize.y - furnaceHeight) * 0.5f;
    
    ImGui::SetNextWindowSize(ImVec2(inventoryWidth, furnaceHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always);
    
    // Style the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.2f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
    
    if (ImGui::Begin("Furnace", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                     ImGuiWindowFlags_NoCollapse)) {
        
        // Furnace area
        ImGui::Text("Furnace");
        ImGui::Separator();
        
        // Simple 3-slot furnace layout: Input -> Fuel -> Output
        for (int i = 0; i < 3; i++) {
            if (i > 0) ImGui::SameLine();
            
            std::string slotId = "furnace_" + std::to_string(i);
            ImGui::PushID(slotId.c_str());
            
            // Draw empty furnace slot
            ImGui::Button("", ImVec2(slotSize, slotSize));
            
            ImGui::PopID();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (m_player) {
            const auto& inventory = m_player->GetInventory();
            
            // Main inventory (3 rows of 9)
            ImGui::Text("Inventory");
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 9; col++) {
                    if (col > 0) ImGui::SameLine(0, slotSpacing);
                    
                    int slotIndex = row * 9 + col;
                    const InventorySlot& slot = inventory.getSlot(slotIndex);
                    RenderCustomInventorySlot(slot, 0, 0, slotSize, slotIndex);
                }
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Hotbar
            ImGui::Text("Hotbar");
            for (int i = 0; i < 9; i++) {
                if (i > 0) ImGui::SameLine(0, slotSpacing);
                
                int slotIndex = 27 + i; // Hotbar starts at slot 27
                const InventorySlot& slot = inventory.getSlot(slotIndex);
                bool isSelected = (i == m_selectedHotbarSlot);
                RenderCustomHotbarSlot(slot, 0, 0, slotSize, slotIndex, isSelected);
            }
        }
    }
    ImGui::End();
    
    // Pop all style modifications
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

