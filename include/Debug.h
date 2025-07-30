#pragma once

#include <iostream>

// Debug configuration
#ifndef DEBUG_MODE
    #ifdef _DEBUG
        #define DEBUG_MODE 1
    #else
        #define DEBUG_MODE 0
    #endif
#endif

// Debug levels
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

// Set the current debug level (can be changed at compile time)
#ifndef DEBUG_LEVEL
    #if DEBUG_MODE
        #define DEBUG_LEVEL DEBUG_LEVEL_WARNING  // Show errors and warnings in debug builds
    #else
        #define DEBUG_LEVEL DEBUG_LEVEL_ERROR    // Only show errors in release builds
    #endif
#endif

// Debug macros
#define DEBUG_ERROR(msg) \
    do { if (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR) { std::cerr << "[ERROR] " << msg << std::endl; } } while(0)

#define DEBUG_WARNING(msg) \
    do { if (DEBUG_LEVEL >= DEBUG_LEVEL_WARNING) { std::cout << "[WARNING] " << msg << std::endl; } } while(0)

#define DEBUG_INFO(msg) \
    do { if (DEBUG_LEVEL >= DEBUG_LEVEL_INFO) { std::cout << "[INFO] " << msg << std::endl; } } while(0)

#define DEBUG_VERBOSE(msg) \
    do { if (DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) { std::cout << "[VERBOSE] " << msg << std::endl; } } while(0)

// Conditional debug macros for specific systems
#define DEBUG_TEXTURE_LOADING   0   // Set to 1 to enable texture loading messages
#define DEBUG_SHADER_LOADING    0   // Set to 1 to enable shader loading messages  
#define DEBUG_BLOCK_LOADING     1   // Set to 1 to enable block loading messages
#define DEBUG_NETWORK_VERBOSE   0   // Set to 1 to enable verbose network messages
#define DEBUG_INVENTORY_LOADING 0   // Set to 1 to enable inventory loading messages

// System-specific debug macros
#define DEBUG_TEXTURE(msg) \
    do { if (DEBUG_TEXTURE_LOADING) { std::cout << "[TEXTURE] " << msg << std::endl; } } while(0)

#define DEBUG_SHADER(msg) \
    do { if (DEBUG_SHADER_LOADING) { std::cout << "[SHADER] " << msg << std::endl; } } while(0)

#define DEBUG_BLOCKS(msg) \
    do { if (DEBUG_BLOCK_LOADING) { std::cout << "[BLOCKS] " << msg << std::endl; } } while(0)

#define DEBUG_NETWORK(msg) \
    do { if (DEBUG_NETWORK_VERBOSE) { std::cout << "[NETWORK] " << msg << std::endl; } } while(0)

#define DEBUG_INVENTORY(msg) \
    do { if (DEBUG_INVENTORY_LOADING) { std::cout << "[INVENTORY] " << msg << std::endl; } } while(0)