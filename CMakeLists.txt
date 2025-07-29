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
find_package(PkgConfig QUIET)

# Find GLFW
find_package(glfw3 QUIET)
if(NOT TARGET glfw)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLFW QUIET glfw3)
        if(GLFW_FOUND)
            add_library(glfw INTERFACE)
            target_link_libraries(glfw INTERFACE ${GLFW_LIBRARIES})
            target_include_directories(glfw INTERFACE ${GLFW_INCLUDE_DIRS})
            target_compile_options(glfw INTERFACE ${GLFW_CFLAGS_OTHER})
        endif()
    endif()
endif()

# Find OpenGL loading library (prefer epoxy, fallback to GLEW or GLAD)
set(OPENGL_LOADER_FOUND FALSE)

# Try to find libepoxy first
if(PkgConfig_FOUND)
    pkg_check_modules(EPOXY QUIET epoxy)
    if(EPOXY_FOUND)
        set(OPENGL_LOADER_FOUND TRUE)
        set(OPENGL_LOADER_NAME "epoxy")
        add_library(opengl_loader INTERFACE)
        target_link_libraries(opengl_loader INTERFACE ${EPOXY_LIBRARIES})
        target_include_directories(opengl_loader INTERFACE ${EPOXY_INCLUDE_DIRS})
        target_compile_options(opengl_loader INTERFACE ${EPOXY_CFLAGS_OTHER})
        target_compile_definitions(opengl_loader INTERFACE USE_EPOXY)
    endif()
endif()

# Fallback to find_library for epoxy if pkg-config didn't work
if(NOT OPENGL_LOADER_FOUND)
    find_library(EPOXY_LIBRARY NAMES epoxy)
    find_path(EPOXY_INCLUDE_DIR NAMES epoxy/gl.h)
    if(EPOXY_LIBRARY AND EPOXY_INCLUDE_DIR)
        set(OPENGL_LOADER_FOUND TRUE)
        set(OPENGL_LOADER_NAME "epoxy")
        add_library(opengl_loader INTERFACE)
        target_link_libraries(opengl_loader INTERFACE ${EPOXY_LIBRARY})
        target_include_directories(opengl_loader INTERFACE ${EPOXY_INCLUDE_DIR})
        target_compile_definitions(opengl_loader INTERFACE USE_EPOXY)
    endif()
endif()

# Try GLEW as fallback
if(NOT OPENGL_LOADER_FOUND)
    find_package(GLEW QUIET)
    if(GLEW_FOUND)
        set(OPENGL_LOADER_FOUND TRUE)
        set(OPENGL_LOADER_NAME "GLEW")
        add_library(opengl_loader INTERFACE)
        target_link_libraries(opengl_loader INTERFACE GLEW::GLEW)
        target_compile_definitions(opengl_loader INTERFACE USE_GLEW)
    endif()
endif()

# Final fallback: no loader library (use system OpenGL headers)
if(NOT OPENGL_LOADER_FOUND)
    message(WARNING "No OpenGL loading library found (epoxy, GLEW). Using system OpenGL headers.")
    set(OPENGL_LOADER_NAME "system")
    add_library(opengl_loader INTERFACE)
    target_compile_definitions(opengl_loader INTERFACE USE_SYSTEM_OPENGL)
endif()

message(STATUS "Using OpenGL loader: ${OPENGL_LOADER_NAME}")

# Verify we have GLFW
if(NOT TARGET glfw AND NOT GLFW_FOUND)
    message(FATAL_ERROR "GLFW not found. Please install GLFW development libraries.")
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
    src/PlayerModel.cpp
    src/Server.cpp
    src/NetworkClient.cpp
    src/ServerDiscovery.cpp
)

# Header files
set(HEADERS
    include/Game.h
    include/Renderer.h
    include/Block.h
    include/Chunk.h
    include/World.h
    include/Player.h
    include/PlayerModel.h
    include/Server.h
    include/NetworkClient.h
    include/ServerDiscovery.h
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
    opengl_loader
)

# Link GLFW
if(TARGET glfw)
    target_link_libraries(${PROJECT_NAME} glfw)
elseif(GLFW_FOUND)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME} PRIVATE ${GLFW_CFLAGS_OTHER})
endif()

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

# Copy assets and shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}) 