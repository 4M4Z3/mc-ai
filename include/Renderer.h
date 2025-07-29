#pragma once

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();
    void Clear();
    void SetViewport(int width, int height);
    
    // Triangle rendering
    void RenderTriangle();

private:
    // Triangle data
    unsigned int m_VAO;  // Vertex Array Object
    unsigned int m_VBO;  // Vertex Buffer Object
    unsigned int m_shaderProgram;

    // Shader management
    bool CreateShaders();
    unsigned int CompileShader(unsigned int type, const char* source);
    bool CheckShaderCompilation(unsigned int shader, const char* type);
    bool CheckProgramLinking(unsigned int program);
}; 