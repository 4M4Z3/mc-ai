#include "Item.h"
#include "Debug.h"
#include <fstream>
#include <iostream>

ItemManager::ItemManager() {
    // Initialize with some hardcoded test items for now
    InitializeTestItems();
}

bool ItemManager::loadFromConfig(const std::string& configPath) {
    // For now, just return true since we have test items initialized
    // In the future, we can implement proper JSON parsing here
    DEBUG_INVENTORY("Using hardcoded test items (JSON loading not implemented yet)");
    DEBUG_INVENTORY("Loaded " << items.size() << " test items");
    return true;
}

Item* ItemManager::getItem(const std::string& key) const {
    auto it = items.find(key);
    return (it != items.end()) ? it->second.get() : nullptr;
}

Item* ItemManager::getItemById(int id) const {
    auto it = itemsById.find(id);
    return (it != itemsById.end()) ? it->second : nullptr;
}

void ItemManager::addItem(const std::string& key, std::unique_ptr<Item> item) {
    if (item) {
        itemsById[item->itemId] = item.get();
        items[key] = std::move(item);
    }
}

void ItemManager::InitializeTestItems() {
    // Create some test items for the inventory system
    
    // Diamond sword
    auto diamondSword = std::make_unique<Item>(1, "Diamond Sword", ItemType::ITEM, "item/diamond_sword.png", true, 1);
    itemsById[1] = diamondSword.get();
    items["diamond_sword"] = std::move(diamondSword);
    
    // Diamond pickaxe
    auto diamondPickaxe = std::make_unique<Item>(2, "Diamond Pickaxe", ItemType::ITEM, "item/diamond_pickaxe.png", true, 1);
    itemsById[2] = diamondPickaxe.get();
    items["diamond_pickaxe"] = std::move(diamondPickaxe);
    
    // Cooked beef
    auto cookedBeef = std::make_unique<Item>(3, "Cooked Beef", ItemType::ITEM, "item/cooked_beef.png", true, 64);
    itemsById[3] = cookedBeef.get();
    items["cooked_beef"] = std::move(cookedBeef);
    
    // Oak planks
    auto oakPlanks = std::make_unique<Item>(4, "Oak Planks", ItemType::ITEM, "item/oak_planks.png", true, 64);
    itemsById[4] = oakPlanks.get();
    items["oak_planks"] = std::move(oakPlanks);
    
    // Stone
    auto stone = std::make_unique<Item>(5, "Stone", ItemType::ITEM, "item/stone.png", true, 64);
    itemsById[5] = stone.get();
    items["stone"] = std::move(stone);
    
    // Diamond
    auto diamond = std::make_unique<Item>(6, "Diamond", ItemType::ITEM, "item/diamond.png", true, 64);
    itemsById[6] = diamond.get();
    items["diamond"] = std::move(diamond);
    
    // Apple
    auto apple = std::make_unique<Item>(7, "Apple", ItemType::ITEM, "item/apple.png", true, 64);
    itemsById[7] = apple.get();
    items["apple"] = std::move(apple);
    
    // Bread
    auto bread = std::make_unique<Item>(8, "Bread", ItemType::ITEM, "item/bread.png", true, 64);
    itemsById[8] = bread.get();
    items["bread"] = std::move(bread);
    
    // Water bucket
    auto waterBucket = std::make_unique<Item>(9, "Water Bucket", ItemType::ITEM, "item/water_bucket.png", true, 1);
    itemsById[9] = waterBucket.get();
    items["water_bucket"] = std::move(waterBucket);
}