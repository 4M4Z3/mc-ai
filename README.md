# ImGui OpenGL Game

A cross-platform C++ game framework using ImGui for UI and OpenGL for rendering. Features a simple state management system with a main menu and a game state that renders an OpenGL triangle.

## Features

- **Cross-platform**: Works on Mac M1 and Windows x86_64
- **ImGui Integration**: Modern immediate mode GUI
- **OpenGL Rendering**: Hardware-accelerated graphics
- **State Management**: Clean separation between main menu and game states
- **Input Handling**: ESC key to return from game to main menu

## Project Structure

```
├── src/           # Source files
├── include/       # Header files  
├── bin/           # Built executables
├── lib/           # Static libraries
├── third_party/   # External dependencies (ImGui)
├── assets/        # Game assets
└── CMakeLists.txt # Build configuration
```

## Dependencies

- **GLFW3**: Window management and input handling
- **OpenGL**: Graphics rendering
- **ImGui**: Immediate mode GUI (automatically downloaded during build)

## Build Instructions

### Prerequisites

#### Mac M1 (macOS)
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake glfw
```

#### Windows x86_64
1. Install [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) with C++ development tools
2. Install [CMake](https://cmake.org/download/)
3. Install [vcpkg](https://github.com/Microsoft/vcpkg):
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg.exe install glfw3:x64-windows
   ```

### Building

#### Quick Start (Using Makefile)
```bash
# Clone the project
git clone <your-repo-url>
cd ImGuiOpenGLProject

# Build and run (works on Mac and Linux)
make run

# Or just build
make

# Clean build files
make clean

# See all available commands
make help
```

#### Mac M1 (Manual CMake)
```bash
# Clone and build
git clone <your-repo-url>
cd ImGuiOpenGLProject

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run the executable
./bin/ImGuiOpenGLProject
```

#### Windows x86_64
```cmd
REM Clone and build
git clone <your-repo-url>
cd ImGuiOpenGLProject

REM Create build directory
mkdir build && cd build

REM Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake

REM Build (or open the .sln file in Visual Studio)
cmake --build . --config Release

REM Run the executable
.\bin\Release\ImGuiOpenGLProject.exe
```

### Alternative: Visual Studio Code
1. Install the CMake Tools extension
2. Open the project folder
3. Select your kit (compiler)
4. Press F7 to build or Ctrl+F5 to build and run

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

### Mac M1 Issues
- If you get OpenGL deprecation warnings, they can be safely ignored
- Make sure you have Xcode command line tools: `xcode-select --install`

### Windows Issues
- Ensure vcpkg is properly integrated: `.\vcpkg integrate install`
- If GLFW is not found, verify the vcpkg toolchain path in CMake configuration
- Use x64 architecture for consistency

### General Issues
- Make sure your graphics drivers support OpenGL 3.3+
- ImGui is automatically downloaded during build - ensure you have internet access for first build
- Check that CMake version is 3.15 or higher
- If build fails, try `make clean` followed by `make setup` to re-download dependencies

## License

This project is provided as-is for educational purposes. 