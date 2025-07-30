#include "CraftingSystem.h"
#include "Debug.h"
#include <iostream>

CraftingSystem::CraftingSystem(ItemManager* itemMgr) : itemManager(itemMgr) {
    initializeRecipes();
}

void CraftingSystem::initializeRecipes() {
    if (!itemManager) return;
    
    // Recipe 1: Log -> 4 Wooden Planks (log in bottom left)
    Item* oakLog = itemManager->getItem("oak_log");
    Item* oakPlanks = itemManager->getItem("oak_planks");
    
    if (oakLog && oakPlanks) {
        auto logToPlanksRecipe = std::make_unique<CraftingRecipe>(oakPlanks, 4);
        // Place log in bottom left (row 1, col 0)
        logToPlanksRecipe->setInput(1, 0, oakLog, 1);
        recipes.push_back(std::move(logToPlanksRecipe));
        
        std::cout << "Added recipe: Oak Log -> 4 Oak Planks" << std::endl;
    }
    
    // Add recipes for other log types
    const std::vector<std::pair<std::string, std::string>> logTypes = {
        {"birch_log", "birch_planks"},
        {"dark_oak_log", "spruce_planks"} // Using spruce planks for dark oak for now
    };
    
    for (const auto& logType : logTypes) {
        Item* log = itemManager->getItem(logType.first);
        Item* planks = itemManager->getItem(logType.second);
        
        if (log && planks) {
            auto recipe = std::make_unique<CraftingRecipe>(planks, 4);
            recipe->setInput(1, 0, log, 1); // Bottom left
            recipes.push_back(std::move(recipe));
            
            std::cout << "Added recipe: " << log->itemName << " -> 4 " << planks->itemName << std::endl;
        }
    }
    
    // Recipe 2: 2 Wooden Planks (vertical) -> 4 Sticks
    Item* stick = itemManager->getItem("stick");
    
    if (oakPlanks && stick) {
        auto planksToSticksRecipe = std::make_unique<CraftingRecipe>(stick, 4);
        // Place planks vertically (one above the other in left column)
        planksToSticksRecipe->setInput(0, 0, oakPlanks, 1); // Top left
        planksToSticksRecipe->setInput(1, 0, oakPlanks, 1); // Bottom left
        recipes.push_back(std::move(planksToSticksRecipe));
        
        std::cout << "Added recipe: 2 Oak Planks (vertical) -> 4 Sticks" << std::endl;
    }
    
    // Add stick recipes for other plank types
    const std::vector<std::string> plankTypes = {"birch_planks", "spruce_planks"};
    
    for (const auto& plankType : plankTypes) {
        Item* planks = itemManager->getItem(plankType);
        
        if (planks && stick) {
            auto recipe = std::make_unique<CraftingRecipe>(stick, 4);
            recipe->setInput(0, 0, planks, 1); // Top left
            recipe->setInput(1, 0, planks, 1); // Bottom left
            recipes.push_back(std::move(recipe));
            
            std::cout << "Added recipe: 2 " << planks->itemName << " (vertical) -> 4 Sticks" << std::endl;
        }
    }
    
    // Pickaxe recipes (simplified 2x2 patterns)
    Item* woodenPickaxe = itemManager->getItem("wooden_pickaxe");
    Item* stonePickaxe = itemManager->getItem("stone_pickaxe");
    Item* ironPickaxe = itemManager->getItem("iron_pickaxe");
    Item* diamondPickaxe = itemManager->getItem("diamond_pickaxe");
    
    // Materials
    Item* cobblestone = itemManager->getItem("cobblestone");
    Item* ironIngot = itemManager->getItem("iron_ingot");
    Item* diamond = itemManager->getItem("diamond");
    
    // Wooden Pickaxe: Oak Planks (top row) + Sticks (bottom row)
    if (oakPlanks && stick && woodenPickaxe) {
        auto recipe = std::make_unique<CraftingRecipe>(woodenPickaxe, 1);
        recipe->setInput(0, 0, oakPlanks, 1); // Top left
        recipe->setInput(0, 1, oakPlanks, 1); // Top right
        recipe->setInput(1, 0, stick, 1);     // Bottom left
        recipe->setInput(1, 1, stick, 1);     // Bottom right
        recipes.push_back(std::move(recipe));
        std::cout << "Added recipe: 2 Oak Planks + 2 Sticks -> Wooden Pickaxe" << std::endl;
    }
    
    // Stone Pickaxe: Cobblestone (top row) + Sticks (bottom row)
    if (cobblestone && stick && stonePickaxe) {
        auto recipe = std::make_unique<CraftingRecipe>(stonePickaxe, 1);
        recipe->setInput(0, 0, cobblestone, 1); // Top left
        recipe->setInput(0, 1, cobblestone, 1); // Top right
        recipe->setInput(1, 0, stick, 1);       // Bottom left
        recipe->setInput(1, 1, stick, 1);       // Bottom right
        recipes.push_back(std::move(recipe));
        std::cout << "Added recipe: 2 Cobblestone + 2 Sticks -> Stone Pickaxe" << std::endl;
    }
    
    // Iron Pickaxe: Iron Ingots (top row) + Sticks (bottom row)
    if (ironIngot && stick && ironPickaxe) {
        auto recipe = std::make_unique<CraftingRecipe>(ironPickaxe, 1);
        recipe->setInput(0, 0, ironIngot, 1); // Top left
        recipe->setInput(0, 1, ironIngot, 1); // Top right
        recipe->setInput(1, 0, stick, 1);     // Bottom left
        recipe->setInput(1, 1, stick, 1);     // Bottom right
        recipes.push_back(std::move(recipe));
        std::cout << "Added recipe: 2 Iron Ingots + 2 Sticks -> Iron Pickaxe" << std::endl;
    }
    
    // Diamond Pickaxe: Diamonds (top row) + Sticks (bottom row)
    if (diamond && stick && diamondPickaxe) {
        auto recipe = std::make_unique<CraftingRecipe>(diamondPickaxe, 1);
        recipe->setInput(0, 0, diamond, 1); // Top left
        recipe->setInput(0, 1, diamond, 1); // Top right
        recipe->setInput(1, 0, stick, 1);   // Bottom left
        recipe->setInput(1, 1, stick, 1);   // Bottom right
        recipes.push_back(std::move(recipe));
        std::cout << "Added recipe: 2 Diamonds + 2 Sticks -> Diamond Pickaxe" << std::endl;
    }
    
    std::cout << "Initialized " << recipes.size() << " crafting recipes" << std::endl;
}

const CraftingRecipe* CraftingSystem::findMatchingRecipe(const CraftingRecipe::CraftingSlot craftingSlots[4]) const {
    for (const auto& recipe : recipes) {
        if (recipe->matches(craftingSlots)) {
            return recipe.get();
        }
    }
    return nullptr;
}

CraftingSystem::CraftingResult CraftingSystem::checkCrafting(const CraftingRecipe::CraftingSlot craftingSlots[4]) const {
    const CraftingRecipe* recipe = findMatchingRecipe(craftingSlots);
    
    if (recipe) {
        return CraftingResult(recipe->outputItem, recipe->outputQuantity);
    }
    
    return CraftingResult(); // No match found
}

CraftingSystem::CraftingResult CraftingSystem::performCrafting(CraftingRecipe::CraftingSlot craftingSlots[4]) const {
    const CraftingRecipe* recipe = findMatchingRecipe(craftingSlots);
    
    if (!recipe) {
        return CraftingResult(); // No recipe found
    }
    
    // Check if we have enough materials
    for (int i = 0; i < 4; i++) {
        if (!recipe->inputs[i].isEmpty()) {
            if (craftingSlots[i].quantity < recipe->inputs[i].quantity) {
                return CraftingResult(); // Not enough materials
            }
        }
    }
    
    // Consume ingredients
    for (int i = 0; i < 4; i++) {
        if (!recipe->inputs[i].isEmpty()) {
            craftingSlots[i].quantity -= recipe->inputs[i].quantity;
            if (craftingSlots[i].quantity <= 0) {
                craftingSlots[i].item = nullptr;
                craftingSlots[i].quantity = 0;
            }
        }
    }
    
    std::cout << "Crafted: " << recipe->outputQuantity << "x " << recipe->outputItem->itemName << std::endl;
    
    return CraftingResult(recipe->outputItem, recipe->outputQuantity);
}