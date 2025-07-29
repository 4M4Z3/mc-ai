# Makefile for ImGui OpenGL Project
# Cross-platform build wrapper for CMake

# Project settings
PROJECT_NAME = ImGuiOpenGLProject
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# Detect platform first
UNAME := $(shell uname)

# Add homebrew to PATH for macOS
ifeq ($(UNAME), Darwin)
    export PATH := /opt/homebrew/bin:$(PATH)
    export CC := /usr/bin/clang
    export CXX := /usr/bin/clang++
endif
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
ifeq ($(PLATFORM), macos)
	@cd $(BUILD_DIR) && PATH="/opt/homebrew/bin:$$PATH" CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. -G $(CMAKE_GENERATOR)
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
	@cd $(BUILD_DIR) && PATH="/opt/homebrew/bin:$$PATH" make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
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
	@echo "  install-deps-mac - Install dependencies on macOS"
	@echo "  install-deps-win - Install dependencies on Windows"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Platform detected: $(PLATFORM)"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Executable: $(EXECUTABLE)" 