#pragma once

#include "Item.h"
#include <vector>
#include <memory>

struct CraftingRecipe {
    struct CraftingSlot {
        Item* item;
        int quantity;
        
        CraftingSlot() : item(nullptr), quantity(0) {}
        CraftingSlot(Item* item, int qty) : item(item), quantity(qty) {}
        
        bool isEmpty() const { return item == nullptr || quantity <= 0; }
        bool matches(Item* other, int otherQty) const {
            if (isEmpty() && (other == nullptr || otherQty <= 0)) return true;
            if (isEmpty() || other == nullptr) return false;
            return item->itemId == other->itemId && quantity <= otherQty;
        }
    };
    
    // 2x2 crafting grid (4 slots total)
    CraftingSlot inputs[4];  // [0,1] = top row, [2,3] = bottom row
    
    Item* outputItem;
    int outputQuantity;
    
    CraftingRecipe(Item* output, int outputQty) 
        : outputItem(output), outputQuantity(outputQty) {}
    
    // Helper functions to set input patterns
    void setInput(int row, int col, Item* item, int quantity = 1) {
        if (row >= 0 && row < 2 && col >= 0 && col < 2) {
            inputs[row * 2 + col] = CraftingSlot(item, quantity);
        }
    }
    
    bool matches(const CraftingSlot craftingSlots[4]) const {
        for (int i = 0; i < 4; i++) {
            if (!inputs[i].matches(craftingSlots[i].item, craftingSlots[i].quantity)) {
                return false;
            }
        }
        return true;
    }
};

class CraftingSystem {
private:
    std::vector<std::unique_ptr<CraftingRecipe>> recipes;
    ItemManager* itemManager;
    
public:
    CraftingSystem(ItemManager* itemMgr);
    ~CraftingSystem() = default;
    
    // Initialize default recipes
    void initializeRecipes();
    
    // Try to find a recipe that matches the current crafting grid
    const CraftingRecipe* findMatchingRecipe(const CraftingRecipe::CraftingSlot craftingSlots[4]) const;
    
    // Check if crafting is possible and return result
    struct CraftingResult {
        bool canCraft;
        Item* resultItem;
        int resultQuantity;
        
        CraftingResult() : canCraft(false), resultItem(nullptr), resultQuantity(0) {}
        CraftingResult(Item* item, int qty) : canCraft(true), resultItem(item), resultQuantity(qty) {}
    };
    
    CraftingResult checkCrafting(const CraftingRecipe::CraftingSlot craftingSlots[4]) const;
    
    // Perform crafting - consumes ingredients and returns result
    CraftingResult performCrafting(CraftingRecipe::CraftingSlot craftingSlots[4]) const;
};