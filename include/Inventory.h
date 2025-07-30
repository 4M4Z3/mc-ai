#pragma once

#include "Item.h"
#include <vector>
#include <memory>

struct InventorySlot {
    Item* item;      // Pointer to the item type
    int quantity;    // How many of this item (0 means empty slot)
    
    InventorySlot() : item(nullptr), quantity(0) {}
    InventorySlot(Item* itemPtr, int qty) : item(itemPtr), quantity(qty) {}
    
    bool isEmpty() const { return item == nullptr || quantity <= 0; }
    bool isFull() const { return item != nullptr && quantity >= item->maxStackSize; }
    bool canStack(Item* other) const {
        return item != nullptr && other != nullptr && 
               item->itemId == other->itemId && 
               item->stackable && quantity < item->maxStackSize;
    }
    
    // Try to add items to this slot, returns the number that couldn't fit
    int addItems(Item* itemToAdd, int quantityToAdd) {
        if (isEmpty()) {
            // Empty slot, can accept any item
            item = itemToAdd;
            quantity = std::min(quantityToAdd, itemToAdd->maxStackSize);
            return quantityToAdd - quantity;
        } else if (canStack(itemToAdd)) {
            // Can stack with existing item
            int spaceAvailable = item->maxStackSize - quantity;
            int amountToAdd = std::min(quantityToAdd, spaceAvailable);
            quantity += amountToAdd;
            return quantityToAdd - amountToAdd;
        } else {
            // Can't add to this slot
            return quantityToAdd;
        }
    }
    
    // Remove items from this slot, returns the number actually removed
    int removeItems(int quantityToRemove) {
        if (isEmpty()) {
            return 0;
        }
        
        int amountRemoved = std::min(quantityToRemove, quantity);
        quantity -= amountRemoved;
        
        if (quantity <= 0) {
            item = nullptr;
            quantity = 0;
        }
        
        return amountRemoved;
    }
    
    void clear() {
        item = nullptr;
        quantity = 0;
    }
};

class Inventory {
public:
    static const int MAIN_INVENTORY_SIZE = 27;    // 3 rows of 9 slots each
    static const int HOTBAR_SIZE = 9;             // 1 row of 9 slots
    static const int CRAFTING_GRID_SIZE = 4;      // 2x2 crafting grid
    static const int CRAFTING_RESULT_SIZE = 1;    // Result slot
    static const int TOTAL_SIZE = MAIN_INVENTORY_SIZE + HOTBAR_SIZE + CRAFTING_GRID_SIZE + CRAFTING_RESULT_SIZE + 1; // +1 for cursor slot
    
    enum SlotType {
        MAIN_INVENTORY_START = 0,
        MAIN_INVENTORY_END = MAIN_INVENTORY_SIZE - 1,
        HOTBAR_START = MAIN_INVENTORY_SIZE,
        HOTBAR_END = MAIN_INVENTORY_SIZE + HOTBAR_SIZE - 1,
        CRAFTING_GRID_START = MAIN_INVENTORY_SIZE + HOTBAR_SIZE,
        CRAFTING_GRID_END = MAIN_INVENTORY_SIZE + HOTBAR_SIZE + CRAFTING_GRID_SIZE - 1,
        CRAFTING_RESULT_SLOT = MAIN_INVENTORY_SIZE + HOTBAR_SIZE + CRAFTING_GRID_SIZE,
        CURSOR_SLOT = MAIN_INVENTORY_SIZE + HOTBAR_SIZE + CRAFTING_GRID_SIZE + CRAFTING_RESULT_SIZE
    };
    
    Inventory();
    ~Inventory() = default;
    
    // Basic slot access
    InventorySlot& getSlot(int index) { return slots[index]; }
    const InventorySlot& getSlot(int index) const { return slots[index]; }
    
    // Hotbar specific access
    InventorySlot& getHotbarSlot(int hotbarIndex) { return slots[HOTBAR_START + hotbarIndex]; }
    const InventorySlot& getHotbarSlot(int hotbarIndex) const { return slots[HOTBAR_START + hotbarIndex]; }
    
    // Cursor slot access
    InventorySlot& getCursorSlot() { return slots[CURSOR_SLOT]; }
    const InventorySlot& getCursorSlot() const { return slots[CURSOR_SLOT]; }
    
    // Crafting grid access (2x2)
    InventorySlot& getCraftingSlot(int craftingIndex) { return slots[CRAFTING_GRID_START + craftingIndex]; }
    const InventorySlot& getCraftingSlot(int craftingIndex) const { return slots[CRAFTING_GRID_START + craftingIndex]; }
    
    // Get crafting slot by row, col (0-1 for each)
    InventorySlot& getCraftingSlot(int row, int col) { return slots[CRAFTING_GRID_START + (row * 2) + col]; }
    const InventorySlot& getCraftingSlot(int row, int col) const { return slots[CRAFTING_GRID_START + (row * 2) + col]; }
    
    // Crafting result slot access
    InventorySlot& getCraftingResultSlot() { return slots[CRAFTING_RESULT_SLOT]; }
    const InventorySlot& getCraftingResultSlot() const { return slots[CRAFTING_RESULT_SLOT]; }
    
    // Utility functions
    bool addItem(Item* item, int quantity);  // Try to add item to inventory, returns true if all items fit
    int findEmptySlot() const;               // Returns index of first empty slot, -1 if none
    int countItem(Item* item) const;         // Count total quantity of specific item in inventory
    
    // Clear inventory
    void clear();
    
    // Test function to populate hotbar with items
    void populateTestHotbar(ItemManager* itemManager);
    
private:
    std::vector<InventorySlot> slots;
};