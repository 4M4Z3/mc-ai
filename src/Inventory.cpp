#include "Inventory.h"
#include "Debug.h"
#include <iostream>
#include <random>

Inventory::Inventory() : slots(TOTAL_SIZE) {
    // Initialize all slots as empty
    for (auto& slot : slots) {
        slot.clear();
    }
}

bool Inventory::addItem(Item* item, int quantity) {
    if (!item || quantity <= 0) {
        return false;
    }
    
    int remainingQuantity = quantity;
    
    // First try to stack with existing items
    for (int i = 0; i < TOTAL_SIZE - 1; ++i) { // -1 to skip cursor slot
        if (slots[i].canStack(item)) {
            remainingQuantity = slots[i].addItems(item, remainingQuantity);
            if (remainingQuantity <= 0) {
                return true; // All items added successfully
            }
        }
    }
    
    // Then try to fill empty slots
    for (int i = 0; i < TOTAL_SIZE - 1; ++i) { // -1 to skip cursor slot
        if (slots[i].isEmpty()) {
            remainingQuantity = slots[i].addItems(item, remainingQuantity);
            if (remainingQuantity <= 0) {
                return true; // All items added successfully
            }
        }
    }
    
    // Return true only if all items were added
    return remainingQuantity <= 0;
}

int Inventory::findEmptySlot() const {
    for (int i = 0; i < TOTAL_SIZE - 1; ++i) { // -1 to skip cursor slot
        if (slots[i].isEmpty()) {
            return i;
        }
    }
    return -1; // No empty slot found
}

int Inventory::countItem(Item* item) const {
    if (!item) {
        return 0;
    }
    
    int totalCount = 0;
    for (int i = 0; i < TOTAL_SIZE - 1; ++i) { // -1 to skip cursor slot
        if (!slots[i].isEmpty() && slots[i].item->itemId == item->itemId) {
            totalCount += slots[i].quantity;
        }
    }
    return totalCount;
}

void Inventory::clear() {
    for (auto& slot : slots) {
        slot.clear();
    }
}

void Inventory::populateTestHotbar(ItemManager* itemManager) {
    if (!itemManager) {
        std::cerr << "ItemManager is null, cannot populate test hotbar" << std::endl;
        return;
    }
    
    // Clear entire inventory first
    clear();
    
    // List of items to test with (using item keys from the config)
    std::vector<std::string> testItems = {
        "diamond_sword",
        "diamond_pickaxe", 
        "cooked_beef",
        "oak_planks",
        "stone",
        "diamond",
        "apple",
        "bread",
        "water_bucket"
    };
    
    // Random number generator for quantities
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> quantityDist(1, 64);
    
    // Populate hotbar slots with test items
    for (int i = 0; i < HOTBAR_SIZE && i < static_cast<int>(testItems.size()); ++i) {
        Item* testItem = itemManager->getItem(testItems[i]);
        if (testItem) {
            int quantity = quantityDist(gen);
            // Make sure quantity doesn't exceed max stack size
            quantity = std::min(quantity, testItem->maxStackSize);
            
            slots[HOTBAR_START + i] = InventorySlot(testItem, quantity);
            DEBUG_INVENTORY("Added " << quantity << "x " << testItem->itemName 
                      << " to hotbar slot " << i);
        } else {
            DEBUG_WARNING("Could not find item: " << testItems[i]);
        }
    }
    
    // Also populate some main inventory slots with additional items
    std::vector<std::string> mainInventoryItems = {
        "diamond_sword", "diamond_pickaxe", "cooked_beef", "oak_planks", "stone",
        "diamond", "apple", "bread", "water_bucket", "diamond_sword",
        "cooked_beef", "oak_planks", "stone", "diamond", "apple",
        "bread", "diamond_pickaxe", "water_bucket", "stone", "diamond",
        "apple", "cooked_beef", "oak_planks", "bread", "diamond_sword",
        "diamond_pickaxe", "water_bucket"
    };
    
    // Populate main inventory slots (first 27 slots)
    for (int i = 0; i < MAIN_INVENTORY_SIZE && i < static_cast<int>(mainInventoryItems.size()); ++i) {
        Item* testItem = itemManager->getItem(mainInventoryItems[i]);
        if (testItem) {
            int quantity = quantityDist(gen);
            quantity = std::min(quantity, testItem->maxStackSize);
            
            slots[i] = InventorySlot(testItem, quantity);
            DEBUG_INVENTORY("Added " << quantity << "x " << testItem->itemName 
                      << " to main inventory slot " << i);
        } else {
            DEBUG_WARNING("Could not find item: " << mainInventoryItems[i]);
        }
    }
}