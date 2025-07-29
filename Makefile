# Makefile for ImGui OpenGL Project
# Cross-platform build wrapper for CMake

# Project settings
PROJECT_NAME = ImGuiOpenGLProject
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# Detect platform first
UNAME := $(shell uname)

# Platform-specific settings
ifeq ($(UNAME), Darwin)
    PLATFORM = macos
    EXECUTABLE = $(BIN_DIR)/$(PROJECT_NAME)
    CMAKE_GENERATOR = "Unix Makefiles"
    # Set up Homebrew paths for M1 Macs
    HOMEBREW_PREFIX := $(shell if [ -d "/opt/homebrew" ]; then echo "/opt/homebrew"; elif [ -d "/usr/local" ]; then echo "/usr/local"; else echo ""; fi)
    ifneq ($(HOMEBREW_PREFIX),)
        export PATH := $(HOMEBREW_PREFIX)/bin:$(PATH)
        export PKG_CONFIG_PATH := $(HOMEBREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
        export CMAKE_PREFIX_PATH := $(HOMEBREW_PREFIX):$(CMAKE_PREFIX_PATH)
    endif
    export CC := /usr/bin/clang
    export CXX := /usr/bin/clang++
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
ifeq ($(PLATFORM), macos)
	@cd $(BUILD_DIR) && env PATH="$(PATH)" PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" CMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)" CC="$(CC)" CXX="$(CXX)" cmake .. -G $(CMAKE_GENERATOR)
else
	@cd $(BUILD_DIR) && cmake .. -G $(CMAKE_GENERATOR)
endif

# Build the project
.PHONY: build
build: setup configure
	@echo "Building $(PROJECT_NAME)..."
ifeq ($(PLATFORM), windows)
	@cd $(BUILD_DIR) && cmake --build . --config Release
else ifeq ($(PLATFORM), macos)
	@cd $(BUILD_DIR) && env PATH="$(PATH)" make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
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

# Install dependencies
.PHONY: install-deps
install-deps:
ifeq ($(PLATFORM), macos)
	@echo "Installing dependencies for macOS..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install cmake glfw libepoxy pkg-config; \
	else \
		echo "Error: Homebrew not found. Please install Homebrew first:"; \
		echo '/bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'; \
		exit 1; \
	fi
else ifeq ($(PLATFORM), linux)
	@echo "Installing dependencies for Linux..."
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y cmake libglfw3-dev libepoxy-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev pkg-config; \
	elif command -v dnf >/dev/null 2>&1; then \
		sudo dnf install -y cmake glfw-devel libepoxy-devel mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel pkgconf-pkg-config; \
	elif command -v pacman >/dev/null 2>&1; then \
		sudo pacman -S --needed cmake glfw libepoxy mesa libx11 libxrandr libxi pkgconf; \
	else \
		echo "Error: No supported package manager found (apt-get, dnf, or pacman)"; \
		exit 1; \
	fi
else ifeq ($(PLATFORM), windows)
	@echo "Installing dependencies for Windows..."
	@echo "Make sure vcpkg is installed and integrated"
	@vcpkg install glfw3:x64-windows libepoxy:x64-windows
endif

# Legacy install targets for backwards compatibility
.PHONY: install-deps-mac
install-deps-mac: install-deps

.PHONY: install-deps-win  
install-deps-win: install-deps

# Setup project (clone dependencies, etc.)
.PHONY: setup
setup:
	@echo "Setting up project..."
	@mkdir -p third_party
	@if [ ! -d "third_party/imgui" ]; then \
		echo "Downloading ImGui..."; \
		cd third_party && git clone https://github.com/ocornut/imgui.git; \
	else \
		echo "ImGui already exists"; \
	fi
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
	@echo "  install-deps     - Install system dependencies"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Platform detected: $(PLATFORM)"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Executable: $(EXECUTABLE)"
ifeq ($(PLATFORM), macos)
	@echo "Homebrew prefix: $(HOMEBREW_PREFIX)"
endif 