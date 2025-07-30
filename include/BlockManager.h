#pragma once

#include "Block.h"
#include <unordered_map>
#include <string>
#include <vector>

struct BlockTextureInfo {
    std::string all;           // Single texture for all faces
    std::string top;           // Top face  
    std::string bottom;        // Bottom face
    std::string sides;         // Side faces
    std::string overlay;       // Overlay texture (like grass)
    float tintR = 1.0f;        // Color tint RGB
    float tintG = 1.0f;
    float tintB = 1.0f;
    bool hasOverlay = false;   // Needs overlay rendering
};

enum class BlockCategory {
    SOLID,
    TRANSPARENT,
    GROUND
};

struct BlockDefinition {
    std::string blockKey;
    std::string blockName;
    BlockType blockType;
    BlockCategory category = BlockCategory::SOLID;  // Default to solid
    BlockTextureInfo textures;
};

class BlockManager {
public:
    BlockManager();
    ~BlockManager();
    
    // Load block definitions from JSON file
    bool LoadBlockDefinitions(const std::string& jsonPath);
    
    // Block lookup functions
    const BlockDefinition* GetBlockDefinition(BlockType blockType) const;
    const BlockDefinition* GetBlockDefinition(const std::string& blockKey) const;
    BlockType GetBlockTypeByKey(const std::string& blockKey) const;
    const std::string& GetBlockNameByType(BlockType blockType) const;
    
    // Get all available block types
    std::vector<BlockType> GetAllBlockTypes() const;
    
    // Check if a block type exists
    bool IsValidBlockType(BlockType blockType) const;
    
    // Get texture information
    const BlockTextureInfo& GetTextureInfo(BlockType blockType) const;
    
    // Block category checks
    BlockCategory GetBlockCategory(BlockType blockType) const;
    bool IsTransparent(BlockType blockType) const;
    bool IsGround(BlockType blockType) const;
    bool IsSolid(BlockType blockType) const;
    
private:
    std::unordered_map<BlockType, BlockDefinition> m_blocksByType;
    std::unordered_map<std::string, BlockType> m_blocksByKey;
    
    // Default fallback for unknown blocks
    BlockDefinition m_defaultBlock;
    
    // Helper functions
    void InitializeDefaultBlocks();
    bool ParseJsonContent(const std::string& jsonContent);
    bool ParseBlocksSection(const std::string& blocksContent);
    void AddBlock(const std::string& blockKey, const BlockDefinition& blockDef);
};
