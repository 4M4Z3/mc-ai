#include "Game.h"
#include <iostream>
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

Game::Game() : m_window(nullptr), m_currentState(GameState::MAIN_MENU), m_shouldClose(false) {
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

    std::cout << "Game initialized successfully!" << std::endl;
    return true;
}

void Game::Run() {
    while (!glfwWindowShouldClose(m_window) && !m_shouldClose) {
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
}

void Game::SetState(GameState newState) {
    m_currentState = newState;
    std::cout << "State changed to: " << (newState == GameState::MAIN_MENU ? "MAIN_MENU" : "GAME") << std::endl;
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
    // Render the triangle
    m_renderer.RenderTriangle();
    
    // Show game UI
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
    
    if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Triangle Renderer");
        ImGui::Text("Press ESC to return to menu");
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

void Game::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (s_instance) {
        s_instance->m_renderer.SetViewport(width, height);
    }
}

void Game::ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
} 