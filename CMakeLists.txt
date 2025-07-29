cmake_minimum_required(VERSION 3.15)
project(ImGuiOpenGLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# If packages are not found, try to use vcpkg or build from source
if(NOT TARGET glfw)
    message(STATUS "GLFW not found, trying to find it in third_party or system")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui/backends)

# Source files
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Renderer.cpp
    src/Block.cpp
    src/Chunk.cpp
    src/World.cpp
    src/Player.cpp
    src/Server.cpp
    src/NetworkClient.cpp
)

# Header files
set(HEADERS
    include/Game.h
    include/Renderer.h
    include/Block.h
    include/Chunk.h
    include/World.h
    include/Player.h
    include/Server.h
    include/NetworkClient.h
)

# Add ImGui source files
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL
    glfw
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(${PROJECT_NAME} 
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME}
        opengl32
        ws2_32
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME}
        GL
        X11
        pthread
        Xrandr
        Xi
        dl
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}) 