#include "Game.h"
#include <iostream>
#include <ctime>
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#endif

// ImGui includes
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// Static instance for callbacks
Game* Game::s_instance = nullptr;

Game::Game() : m_window(nullptr), m_currentState(GameState::MAIN_MENU), m_shouldClose(false),
               m_firstMouse(true), m_lastX(640.0), m_lastY(360.0), m_deltaTime(0.0f), m_lastFrame(0.0f) {
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
    // Game logic here
}

void Game::RenderMainMenu() {
    // Create a centered window for the main menu
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    
    if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("ImGui OpenGL Game");
        ImGui::Separator();
        
        if (ImGui::Button("Start Game", ImVec2(280, 50))) {
            SetState(GameState::GAME);
        }
        
        if (ImGui::Button("Exit", ImVec2(280, 50))) {
            m_shouldClose = true;
        }
    }
    ImGui::End();
}

void Game::RenderGame() {
    // 3D world rendering
    if (m_world && m_player) {
        m_renderer.BeginFrame(*m_player);
        m_renderer.RenderWorld(*m_world);
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
        
        if (m_world) {
            ImGui::Text("World Information:");
            ImGui::Text("Seed: %d", m_world->GetSeed());
            ImGui::Text("Single block at (0,0,0)");
            
            // Check if block exists
            Block testBlock = m_world->GetBlock(0, 0, 0);
            ImGui::Text("Block at (0,0,0): %s", testBlock.IsAir() ? "Air" : "Solid");
            
            if (ImGui::Button("Regenerate World")) {
                m_world->RegenerateWithSeed(static_cast<int>(std::time(nullptr)));
            }
        } else {
            ImGui::Text("No world loaded");
        }
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("WASD - Move");
        ImGui::Text("Space/Shift - Up/Down");
        ImGui::Text("Mouse - Look around");
        ImGui::Text("ESC - Return to menu");
    }
    ImGui::End();
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