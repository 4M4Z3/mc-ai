#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "BlockTypes.h"

// Forward declaration for nlohmann::json
namespace nlohmann {
    class json;
}

enum class ItemType {
    ITEM,           // Direct PNG sprite items
    BLOCK_ITEM      // Items that represent blocks
};

enum class RenderType {
    ITEM,                // Direct PNG sprite rendering
    ORTHOGRAPHIC_BLOCK,  // 3D orthographic view showing top and two sides
    SPRITE               // 2D sprite rendering from block texture
};

struct ItemTextures {
    std::string all;
    float tintR = 1.0f;
    float tintG = 1.0f;
    float tintB = 1.0f;
    
    // For blocks with different face textures
    std::string top;
    std::string bottom; 
    std::string side;
    std::string front;
    std::string back;
    std::string left;
    std::string right;
};

class Item {
public:
    int itemId;
    std::string itemName;
    ItemType type;
    std::string icon;  // For direct PNG items
    bool stackable;
    int maxStackSize;
    
    // For block items
    std::string blockKey;
    RenderType renderType;
    ItemTextures textures;
    
    Item(int id, const std::string& name, ItemType itemType, 
         const std::string& iconPath, bool isStackable = true, int maxStack = 64)
        : itemId(id), itemName(name), type(itemType), icon(iconPath), 
          stackable(isStackable), maxStackSize(maxStack), 
          renderType(RenderType::ITEM) {}
          
    Item(int id, const std::string& name, const std::string& block, 
         RenderType render, const ItemTextures& tex, bool isStackable = true, int maxStack = 64)
        : itemId(id), itemName(name), type(ItemType::BLOCK_ITEM), 
          stackable(isStackable), maxStackSize(maxStack),
          blockKey(block), renderType(render), textures(tex) {}
          
    virtual ~Item() = default;
    
    bool isBlockItem() const { return type == ItemType::BLOCK_ITEM; }
    bool requiresOrthographicRendering() const { return renderType == RenderType::ORTHOGRAPHIC_BLOCK; }
    bool requiresSpriteRendering() const { return renderType == RenderType::SPRITE; }
};

class ItemManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Item>> items;
    std::unordered_map<int, Item*> itemsById;
    
public:
    ItemManager();
    virtual ~ItemManager() = default;
    
    bool loadFromConfig(const std::string& configPath);
    Item* getItem(const std::string& key) const;
    Item* getItemById(int id) const;
    Item* getItemForBlock(BlockType blockType) const;  // New method
    BlockType getBlockTypeForItem(const std::string& itemKey) const;  // Convert item key to BlockType for placement
    
    const std::unordered_map<std::string, std::unique_ptr<Item>>& getAllItems() const { return items; }
    
    void addItem(const std::string& key, std::unique_ptr<Item> item);
    
    size_t getItemCount() const { return items.size(); }
    
private:
    void InitializeTestItems();
};