#include "BlockManager.h"
#include "Debug.h"
#include <fstream>
#include <iostream>
#include <sstream>

BlockManager::BlockManager() {
    InitializeDefaultBlocks();
}

BlockManager::~BlockManager() {
}

void BlockManager::InitializeDefaultBlocks() {
    // Initialize the existing hardcoded blocks for compatibility
    
    // AIR block
    BlockDefinition airBlock;
    airBlock.blockKey = "air";
    airBlock.blockName = "Air";
    airBlock.blockType = BlockType::AIR;
    airBlock.textures.all = "";
    m_blocksByType[BlockType::AIR] = airBlock;
    m_blocksByKey["air"] = BlockType::AIR;
    
    // STONE block  
    BlockDefinition stoneBlock;
    stoneBlock.blockKey = "stone";
    stoneBlock.blockName = "Stone";
    stoneBlock.blockType = BlockType::STONE;
    stoneBlock.textures.all = "stone.png";
    m_blocksByType[BlockType::STONE] = stoneBlock;
    m_blocksByKey["stone"] = BlockType::STONE;
    
    // DIRT block
    BlockDefinition dirtBlock;
    dirtBlock.blockKey = "dirt";
    dirtBlock.blockName = "Dirt";
    dirtBlock.blockType = BlockType::DIRT;
    dirtBlock.textures.all = "dirt.png";
    m_blocksByType[BlockType::DIRT] = dirtBlock;
    m_blocksByKey["dirt"] = BlockType::DIRT;
    
    // GRASS block (special case with multiple textures)
    BlockDefinition grassBlock;
    grassBlock.blockKey = "grass";
    grassBlock.blockName = "Grass Block";
    grassBlock.blockType = BlockType::GRASS;
    grassBlock.textures.top = "grass_block_top.png";
    grassBlock.textures.sides = "grass_block_side.png"; 
    grassBlock.textures.bottom = "dirt.png";
    grassBlock.textures.overlay = "grass_block_side_overlay.png";
    grassBlock.textures.hasOverlay = true;
    // Tinting will be applied dynamically based on biome
    grassBlock.textures.tintR = 1.0f;
    grassBlock.textures.tintG = 1.0f;
    grassBlock.textures.tintB = 1.0f;
    m_blocksByType[BlockType::GRASS] = grassBlock;
    m_blocksByKey["grass"] = BlockType::GRASS;
    
    // OAK_LOG block (special case with multiple textures)
    BlockDefinition oakLogBlock;
    oakLogBlock.blockKey = "oak_log";
    oakLogBlock.blockName = "Oak Log";
    oakLogBlock.blockType = BlockType::OAK_LOG;
    oakLogBlock.textures.top = "oak_log_top.png";
    oakLogBlock.textures.sides = "oak_log.png";
    oakLogBlock.textures.bottom = "oak_log_top.png";  // Same as top
    m_blocksByType[BlockType::OAK_LOG] = oakLogBlock;
    m_blocksByKey["oak_log"] = BlockType::OAK_LOG;
    
    // BIRCH_LOG block (special case with multiple textures)
    BlockDefinition birchLogBlock;
    birchLogBlock.blockKey = "birch_log";
    birchLogBlock.blockName = "Birch Log";
    birchLogBlock.blockType = BlockType::BIRCH_LOG;
    birchLogBlock.textures.top = "birch_log_top.png";
    birchLogBlock.textures.sides = "birch_log.png";
    birchLogBlock.textures.bottom = "birch_log_top.png";  // Same as top
    m_blocksByType[BlockType::BIRCH_LOG] = birchLogBlock;
    m_blocksByKey["birch_log"] = BlockType::BIRCH_LOG;
    
    // DARK_OAK_LOG block (special case with multiple textures)
    BlockDefinition darkOakLogBlock;
    darkOakLogBlock.blockKey = "dark_oak_log";
    darkOakLogBlock.blockName = "Dark Oak Log";
    darkOakLogBlock.blockType = BlockType::DARK_OAK_LOG;
    darkOakLogBlock.textures.top = "dark_oak_log_top.png";
    darkOakLogBlock.textures.sides = "dark_oak_log.png";
    darkOakLogBlock.textures.bottom = "dark_oak_log_top.png";  // Same as top
    m_blocksByType[BlockType::DARK_OAK_LOG] = darkOakLogBlock;
    m_blocksByKey["dark_oak_log"] = BlockType::DARK_OAK_LOG;

    // Set default block (fallback)
    m_defaultBlock = airBlock;
}

bool BlockManager::LoadBlockDefinitions(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open blocks configuration file: " << jsonPath << std::endl;
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return ParseJsonContent(content);
}

bool BlockManager::ParseJsonContent(const std::string& jsonContent) {
    // Simple JSON parser for our specific block format
    // This is a simplified parser - in production you'd want a proper JSON library
    
    size_t blocksStart = jsonContent.find("\"blocks\":");
    if (blocksStart == std::string::npos) {
        std::cerr << "No 'blocks' section found in JSON" << std::endl;
        return false;
    }
    
    size_t blocksSectionStart = jsonContent.find("{", blocksStart);
    if (blocksSectionStart == std::string::npos) {
        return false;
    }
    
    // Find the matching closing brace for the blocks section
    int braceCount = 1;
    size_t pos = blocksSectionStart + 1;
    size_t blocksSectionEnd = std::string::npos;
    
    while (pos < jsonContent.length() && braceCount > 0) {
        if (jsonContent[pos] == '{') braceCount++;
        else if (jsonContent[pos] == '}') braceCount--;
        
        if (braceCount == 0) {
            blocksSectionEnd = pos;
            break;
        }
        pos++;
    }
    
    if (blocksSectionEnd == std::string::npos) {
        std::cerr << "Invalid JSON structure" << std::endl;
        return false;
    }
    
    std::string blocksContent = jsonContent.substr(blocksSectionStart + 1, 
                                                   blocksSectionEnd - blocksSectionStart - 1);
    
    return ParseBlocksSection(blocksContent);
}

bool BlockManager::ParseBlocksSection(const std::string& blocksContent) {
    std::istringstream stream(blocksContent);
    std::string line;
    std::string currentBlockKey;
    BlockDefinition currentBlock;
    
    bool inBlock = false;
    bool inTextures = false;
    
    while (std::getline(stream, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        if (line.empty() || line[0] == '/' || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        // Look for block key (e.g., "acacia_leaves": {) but not "textures": {
        if (line.find("\":") != std::string::npos && line.find("{") != std::string::npos && 
            line.find("\"textures\":") == std::string::npos) {
            if (inBlock) {
                // Save previous block
                AddBlock(currentBlockKey, currentBlock);
            }
            
            // Start new block
            size_t quoteStart = line.find("\"");
            size_t quoteEnd = line.find("\"", quoteStart + 1);
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                currentBlockKey = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                currentBlock = BlockDefinition();
                currentBlock.blockKey = currentBlockKey;
                inBlock = true;
                inTextures = false;
            }
        }
        // Look for blockType
        else if (line.find("\"blockType\":") != std::string::npos && inBlock) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                value.erase(0, value.find_first_not_of(" \t,"));
                value.erase(value.find_last_not_of(" \t,") + 1);
                currentBlock.blockType = static_cast<BlockType>(std::stoi(value));
            }
        }
        // Look for blockName  
        else if (line.find("\"blockName\":") != std::string::npos && inBlock) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                // Extract string value between quotes
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.blockName = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            }
        }
        // Look for type (category) field
        else if (line.find("\"type\":") != std::string::npos && inBlock) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                // Extract string value between quotes
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    std::string categoryStr = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    if (categoryStr == "solid") {
                        currentBlock.category = BlockCategory::SOLID;
                    } else if (categoryStr == "transparent") {
                        currentBlock.category = BlockCategory::TRANSPARENT;
                    } else if (categoryStr == "ground") {
                        currentBlock.category = BlockCategory::GROUND;
                    }
                }
            }
        }
        // Look for textures section
        else if (line.find("\"textures\":") != std::string::npos && inBlock) {
            inTextures = true;
        }
        // Look for texture properties
        else if (inTextures && line.find("\"all\":") != std::string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.textures.all = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    // std::cout << "[JSON DEBUG] Found texture for " << currentBlockKey << ": " << currentBlock.textures.all << std::endl;
                }
            }
        }
        else if (inTextures && line.find("\"top\":") != std::string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.textures.top = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            }
        }
        else if (inTextures && line.find("\"bottom\":") != std::string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.textures.bottom = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            }
        }
        else if (inTextures && (line.find("\"sides\":") != std::string::npos || line.find("\"side\":") != std::string::npos)) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.textures.sides = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            }
        }
        else if (inTextures && line.find("\"overlay\":") != std::string::npos) {
            size_t colonPos = line.find(":");
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstQuote = value.find("\"");
                size_t lastQuote = value.find_last_of("\"");
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    currentBlock.textures.overlay = value.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    currentBlock.textures.hasOverlay = true;
                }
            }
        }
        // End of textures section
        else if (inTextures && line.find("}") != std::string::npos) {
            inTextures = false;
        }
        // End of block
        else if (inBlock && line.find("},") != std::string::npos) {
            AddBlock(currentBlockKey, currentBlock);
            inBlock = false;
        }
    }
    
    // Add last block if we ended while in a block
    if (inBlock) {
        AddBlock(currentBlockKey, currentBlock);
    }
    
    DEBUG_BLOCKS("Loaded " << (m_blocksByType.size() - 4) << " new blocks from JSON");
    return true;
}

void BlockManager::AddBlock(const std::string& blockKey, const BlockDefinition& blockDef) {
    m_blocksByType[blockDef.blockType] = blockDef;
    m_blocksByKey[blockKey] = blockDef.blockType;
}

const BlockDefinition* BlockManager::GetBlockDefinition(BlockType blockType) const {
    auto it = m_blocksByType.find(blockType);
    if (it != m_blocksByType.end()) {
        return &it->second;
    }
    return &m_defaultBlock;
}

const BlockDefinition* BlockManager::GetBlockDefinition(const std::string& blockKey) const {
    auto it = m_blocksByKey.find(blockKey);
    if (it != m_blocksByKey.end()) {
        return GetBlockDefinition(it->second);
    }
    return &m_defaultBlock;
}

BlockType BlockManager::GetBlockTypeByKey(const std::string& blockKey) const {
    auto it = m_blocksByKey.find(blockKey);
    if (it != m_blocksByKey.end()) {
        return it->second;
    }
    return BlockType::AIR;
}

const std::string& BlockManager::GetBlockNameByType(BlockType blockType) const {
    const BlockDefinition* def = GetBlockDefinition(blockType);
    return def->blockName;
}

std::vector<BlockType> BlockManager::GetAllBlockTypes() const {
    std::vector<BlockType> types;
    for (const auto& pair : m_blocksByType) {
        types.push_back(pair.first);
    }
    return types;
}

bool BlockManager::IsValidBlockType(BlockType blockType) const {
    return m_blocksByType.find(blockType) != m_blocksByType.end();
}

const BlockTextureInfo& BlockManager::GetTextureInfo(BlockType blockType) const {
    const BlockDefinition* def = GetBlockDefinition(blockType);
    return def->textures;
}

BlockCategory BlockManager::GetBlockCategory(BlockType blockType) const {
    const BlockDefinition* def = GetBlockDefinition(blockType);
    return def->category;
}

bool BlockManager::IsTransparent(BlockType blockType) const {
    return GetBlockCategory(blockType) == BlockCategory::TRANSPARENT;
}

bool BlockManager::IsGround(BlockType blockType) const {
    return GetBlockCategory(blockType) == BlockCategory::GROUND;
}

bool BlockManager::IsSolid(BlockType blockType) const {
    return GetBlockCategory(blockType) == BlockCategory::SOLID;
}
