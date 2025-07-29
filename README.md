# ImGui OpenGL Game

A cross-platform C++ game framework using ImGui for UI and OpenGL for rendering. Features a simple state management system with a main menu and a game state that renders an OpenGL triangle.

## Features

- **Cross-platform**: Works on **Linux x86_64** and **Mac M1/M2** (Intel Macs and Windows support available)
- **ImGui Integration**: Modern immediate mode GUI
- **OpenGL Rendering**: Hardware-accelerated graphics with flexible OpenGL loading library support
- **State Management**: Clean separation between main menu and game states
- **Input Handling**: ESC key to return from game to main menu
- **Automated Dependency Management**: Smart detection and fallback for OpenGL loaders (epoxy, GLEW, system)

## Project Structure

```
├── src/           # Source files
├── include/       # Header files  
├── bin/           # Built executables
├── lib/           # Static libraries
├── third_party/   # External dependencies (ImGui)
├── assets/        # Game assets
├── Makefile       # Cross-platform build wrapper
└── CMakeLists.txt # Build configuration
```

## Dependencies

- **CMake**: Build system (version 3.15+)
- **GLFW3**: Window management and input handling
- **OpenGL**: Graphics rendering
- **OpenGL Loading Library**: libepoxy (preferred), GLEW, or system OpenGL headers
- **ImGui**: Immediate mode GUI (automatically downloaded during build)

## Quick Start

### 1. Install Dependencies

The Makefile can automatically install dependencies for your platform:

```bash
# Install dependencies (works on macOS, Ubuntu/Debian, Fedora/RHEL, Arch Linux)
make install-deps
```

**Manual installation:**

#### Mac M1/M2 (macOS)
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake glfw libepoxy pkg-config
```

#### Linux x86_64

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y cmake libglfw3-dev libepoxy-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install -y cmake glfw-devel libepoxy-devel mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel pkgconf-pkg-config
```

**Arch Linux:**
```bash
sudo pacman -S --needed cmake glfw libepoxy mesa libx11 libxrandr libxi pkgconf
```

### 2. Build and Run

```bash
# Clone the project
git clone <your-repo-url>
cd mc-ai

# Build and run in one command
make run

# Or build separately
make build
```

## Advanced Usage

### Build Commands

```bash
make help           # Show all available commands
make clean          # Clean build artifacts  
make configure      # Configure CMake (automatically called by build)
make build          # Build the project
make run            # Build and run the executable
make debug          # Build in debug mode
make setup          # Download ImGui dependency
make install-deps   # Install system dependencies
```

### Manual CMake Build

If you prefer using CMake directly:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./bin/ImGuiOpenGLProject
```

## Cross-Platform Notes

### OpenGL Loading Library Support
The build system automatically detects and uses the best available OpenGL loading library:
1. **libepoxy** (preferred) - Modern, cross-platform
2. **GLEW** (fallback) - Widely supported
3. **System OpenGL** (last resort) - Uses platform OpenGL headers

### Platform-Specific Features
- **macOS**: Automatic Homebrew path detection (supports both `/opt/homebrew` for M1/M2 and `/usr/local` for Intel)
- **Linux**: Multi-distro package manager detection (apt, dnf, pacman)
- **Windows**: vcpkg support for dependency management

## Usage

1. **Main Menu**: 
   - Click "Start Game" to enter the game state
   - Click "Exit" to quit the application

2. **Game State**:
   - View the rendered orange triangle
   - Press ESC to return to the main menu

## Controls

- **ESC**: Return from game state to main menu
- **Mouse**: Interact with ImGui elements

## Troubleshooting

### Build Issues

**"cmake: command not found"**
- Run `make install-deps` to install CMake and other dependencies
- On macOS, ensure Homebrew is installed and in your PATH

**"GLFW not found" or "OpenGL loader not found"**
- Run `make install-deps` to install missing dependencies
- Check that development packages are installed (e.g., `libglfw3-dev` not just `libglfw3`)

**Linker errors with epoxy/GLEW**
- The build system will automatically fall back to available libraries
- Install libepoxy for best compatibility: `brew install libepoxy` (macOS) or `sudo apt install libepoxy-dev` (Ubuntu)

### Runtime Issues

**OpenGL context errors**
- Make sure your graphics drivers support OpenGL 3.3+
- Try running with software rendering: `LIBGL_ALWAYS_SOFTWARE=1 ./build/bin/ImGuiOpenGLProject` (Linux)

**Window creation fails**
- Ensure X11 is running (Linux)
- Check display permissions if running via SSH

### Platform-Specific Issues

**macOS:**
- OpenGL deprecation warnings can be safely ignored
- Ensure Xcode command line tools are installed: `xcode-select --install`

**Linux:**
- Install graphics drivers for your GPU (nvidia, mesa, etc.)
- Make sure you have X11 development libraries installed

**General:**
- ImGui is automatically downloaded - ensure internet access for first build
- Try `make clean && make setup && make build` to refresh dependencies

## Development

### Adding Features
1. Source files go in `src/` with headers in `include/`
2. Update `CMakeLists.txt` to include new source files  
3. Assets go in `assets/` directory (automatically copied to build)

### Debugging
- Use `make debug` for debug builds with symbols
- Enable additional compiler warnings in `CMakeLists.txt`

## License

This project is provided as-is for educational purposes. 