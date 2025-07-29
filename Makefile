# Makefile for ImGui OpenGL Project
# Cross-platform build wrapper for CMake

# Project settings
PROJECT_NAME = ImGuiOpenGLProject
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# Detect platform
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    PLATFORM = macos
    EXECUTABLE = $(BIN_DIR)/$(PROJECT_NAME)
    CMAKE_GENERATOR = "Unix Makefiles"
else ifeq ($(OS), Windows_NT)
    PLATFORM = windows
    EXECUTABLE = $(BIN_DIR)/Release/$(PROJECT_NAME).exe
    CMAKE_GENERATOR = "Visual Studio 16 2019"
else
    PLATFORM = linux
    EXECUTABLE = $(BIN_DIR)/$(PROJECT_NAME)
    CMAKE_GENERATOR = "Unix Makefiles"
endif

# Default target
.PHONY: all
all: build

# Create build directory and configure
.PHONY: configure
configure:
	@echo "Configuring for $(PLATFORM)..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -G $(CMAKE_GENERATOR)

# Build the project
.PHONY: build
build: configure
	@echo "Building $(PROJECT_NAME)..."
ifeq ($(PLATFORM), windows)
	@cd $(BUILD_DIR) && cmake --build . --config Release
else
	@cd $(BUILD_DIR) && make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
endif
	@echo "Build complete!"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete!"

# Run the executable
.PHONY: run
run: build
	@echo "Running $(PROJECT_NAME)..."
	@$(EXECUTABLE)

# Debug build
.PHONY: debug
debug: configure
	@echo "Building $(PROJECT_NAME) in debug mode..."
ifeq ($(PLATFORM), windows)
	@cd $(BUILD_DIR) && cmake --build . --config Debug
else
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
endif
	@echo "Debug build complete!"

# Install dependencies (macOS)
.PHONY: install-deps-mac
install-deps-mac:
	@echo "Installing dependencies for macOS..."
	@brew install cmake glfw || echo "Make sure Homebrew is installed"

# Install dependencies (Windows - requires vcpkg)
.PHONY: install-deps-win
install-deps-win:
	@echo "Installing dependencies for Windows..."
	@echo "Make sure vcpkg is installed and integrated"
	@vcpkg install glfw3:x64-windows

# Setup project (clone dependencies, etc.)
.PHONY: setup
setup:
	@echo "Setting up project..."
	@git submodule update --init --recursive 2>/dev/null || echo "No git submodules to update"
	@echo "Setup complete!"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build the project"
	@echo "  configure        - Configure CMake build"
	@echo "  clean            - Clean build artifacts"
	@echo "  run              - Build and run the executable"
	@echo "  debug            - Build in debug mode"
	@echo "  setup            - Setup project dependencies"
	@echo "  install-deps-mac - Install dependencies on macOS"
	@echo "  install-deps-win - Install dependencies on Windows"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Platform detected: $(PLATFORM)"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Executable: $(EXECUTABLE)" 