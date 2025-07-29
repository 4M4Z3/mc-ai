# Shaders Directory

This directory contains GLSL shader files for the Minecraft clone renderer.

## Current Shaders

### `vertex.glsl`
- Basic 3D vertex shader for block rendering
- Handles model, view, and projection matrix transformations
- Takes vertex positions as input

### `fragment.glsl` 
- Simple fragment shader that renders all blocks in white
- Will be expanded for textures, lighting, and materials

## Usage

Shaders are automatically loaded by the `Renderer::LoadShaderSource()` method at runtime. The renderer expects:
- `shaders/vertex.glsl` - Vertex shader
- `shaders/fragment.glsl` - Fragment shader

## Future Enhancements

- Texture sampling support
- Per-vertex lighting calculations  
- Normal mapping
- Shadow mapping
- Block-specific materials
- Animated textures (water, lava)
- Particle system shaders

## Development Notes

- All shaders use GLSL version 330 core
- Uniform variables: `model`, `view`, `projection` matrices
- Vertex attributes: position (location 0)
- Fragment output: `FragColor` (RGBA) 