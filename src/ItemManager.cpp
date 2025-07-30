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

Item* ItemManager::getItemForBlock(BlockType blockType) const {
    // Comprehensive mapping of BlockType to item keys
    switch (blockType) {
        // Core blocks
        case BlockType::STONE:
            return getItem("stone");
        case BlockType::DIRT:
            return getItem("dirt");
        case BlockType::GRASS:
            return getItem("grass_block");
            
        // Wood logs
        case BlockType::OAK_LOG:
            return getItem("oak_log");
        case BlockType::BIRCH_LOG:
            return getItem("birch_log");
        case BlockType::DARK_OAK_LOG:
            return getItem("dark_oak_log");
            
        // Leaves
        case BlockType::ACACIA_LEAVES:
            return getItem("acacia_leaves");
        case BlockType::AZALEA_LEAVES:
            return getItem("azalea_leaves");
        case BlockType::BIRCH_LEAVES:
            return getItem("birch_leaves");
        case BlockType::CHERRY_LEAVES:
            return getItem("cherry_leaves");
        case BlockType::JUNGLE_LEAVES:
            return getItem("jungle_leaves");
        case BlockType::MANGROVE_LEAVES:
            return getItem("mangrove_leaves");
        case BlockType::SPRUCE_LEAVES:
            return getItem("spruce_leaves");
            
        // Planks
        case BlockType::ACACIA_PLANKS:
            return getItem("acacia_planks");
        case BlockType::BIRCH_PLANKS:
            return getItem("birch_planks");
        case BlockType::CHERRY_PLANKS:
            return getItem("cherry_planks");
        case BlockType::JUNGLE_PLANKS:
            return getItem("jungle_planks");
        case BlockType::SPRUCE_PLANKS:
            return getItem("spruce_planks");
            
        // Common blocks
        case BlockType::ANDESITE:
            return getItem("andesite");
        case BlockType::GRANITE:
            return getItem("granite");
        case BlockType::GRAVEL:
            return getItem("gravel");
        case BlockType::SAND:
            return getItem("sand");
        case BlockType::GLASS:
            return getItem("glass");
        case BlockType::OBSIDIAN:
            return getItem("obsidian");
        case BlockType::BEDROCK:
            return getItem("bedrock");
            
        // Ores
        case BlockType::COPPER_ORE:
            return getItem("copper_ore");
        case BlockType::IRON_ORE:
            return getItem("iron_ore");
        case BlockType::GOLD_ORE:
            return getItem("gold_ore");
        case BlockType::EMERALD_ORE:
            return getItem("emerald_ore");
            
        // Blocks
        case BlockType::COPPER_BLOCK:
            return getItem("copper_block");
        case BlockType::IRON_BLOCK:
            return getItem("iron_block");
        case BlockType::GOLD_BLOCK:
            return getItem("gold_block");
        case BlockType::EMERALD_BLOCK:
            return getItem("emerald_block");
            
        // Wool blocks
        case BlockType::BLUE_WOOL:
            return getItem("blue_wool");
        case BlockType::GRAY_WOOL:
            return getItem("gray_wool");
        case BlockType::GREEN_WOOL:
            return getItem("green_wool");
        case BlockType::LIGHT_BLUE_WOOL:
            return getItem("light_blue_wool");
        case BlockType::LIGHT_GRAY_WOOL:
            return getItem("light_gray_wool");
        case BlockType::ORANGE_WOOL:
            return getItem("orange_wool");
        case BlockType::PINK_WOOL:
            return getItem("pink_wool");
        case BlockType::PURPLE_WOOL:
            return getItem("purple_wool");
        case BlockType::YELLOW_WOOL:
            return getItem("yellow_wool");
            
        // Misc blocks
        case BlockType::BRICKS:
            return getItem("bricks");
        case BlockType::STONE_BRICKS:
            return getItem("stone_bricks");
        case BlockType::GLOWSTONE:
            return getItem("glowstone");
        case BlockType::ICE:
            return getItem("ice");
        case BlockType::SNOW:
            return getItem("snow");
            
        // Special cases that shouldn't be collectable
        case BlockType::AIR:
        case BlockType::WATER_FLOW:
        case BlockType::WATER_STILL:
        case BlockType::NETHER_PORTAL:
            return nullptr; // These blocks shouldn't be collectable
            
        // For unmapped blocks, try to create a generic stone item as fallback
        default:
            // Check if we have a stone item to fall back to
            Item* stoneItem = getItem("stone");
            if (stoneItem) {
                DEBUG_INVENTORY("Warning: Block type " << static_cast<int>(blockType) << " not mapped, using stone as fallback");
                return stoneItem;
            }
            // If even stone doesn't exist, return nullptr
            DEBUG_INVENTORY("Warning: Block type " << static_cast<int>(blockType) << " not mapped and no fallback available");
            return nullptr;
    }
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
    
    // ========== COMPREHENSIVE BLOCK ITEMS ==========
    // Starting from ID 100 for block items
    int itemId = 100;
    
    // Core blocks
    auto dirt = std::make_unique<Item>(itemId++, "Dirt", ItemType::ITEM, "item/dirt.png", true, 64);
    itemsById[dirt->itemId] = dirt.get();
    items["dirt"] = std::move(dirt);
    
    auto grass = std::make_unique<Item>(itemId++, "Grass Block", ItemType::ITEM, "item/grass_block.png", true, 64);
    itemsById[grass->itemId] = grass.get();
    items["grass_block"] = std::move(grass);
    
    // Wood types
    auto oakLog = std::make_unique<Item>(itemId++, "Oak Log", ItemType::ITEM, "item/oak_log.png", true, 64);
    itemsById[oakLog->itemId] = oakLog.get();
    items["oak_log"] = std::move(oakLog);
    
    auto birchLog = std::make_unique<Item>(itemId++, "Birch Log", ItemType::ITEM, "item/birch_log.png", true, 64);
    itemsById[birchLog->itemId] = birchLog.get();
    items["birch_log"] = std::move(birchLog);
    
    auto darkOakLog = std::make_unique<Item>(itemId++, "Dark Oak Log", ItemType::ITEM, "item/dark_oak_log.png", true, 64);
    itemsById[darkOakLog->itemId] = darkOakLog.get();
    items["dark_oak_log"] = std::move(darkOakLog);
    
    // Leaves
    auto acacia_leaves = std::make_unique<Item>(itemId++, "Acacia Leaves", ItemType::ITEM, "item/acacia_leaves.png", true, 64);
    itemsById[acacia_leaves->itemId] = acacia_leaves.get();
    items["acacia_leaves"] = std::move(acacia_leaves);
    
    auto azalea_leaves = std::make_unique<Item>(itemId++, "Azalea Leaves", ItemType::ITEM, "item/azalea_leaves.png", true, 64);
    itemsById[azalea_leaves->itemId] = azalea_leaves.get();
    items["azalea_leaves"] = std::move(azalea_leaves);
    
    auto birch_leaves = std::make_unique<Item>(itemId++, "Birch Leaves", ItemType::ITEM, "item/birch_leaves.png", true, 64);
    itemsById[birch_leaves->itemId] = birch_leaves.get();
    items["birch_leaves"] = std::move(birch_leaves);
    
    auto cherry_leaves = std::make_unique<Item>(itemId++, "Cherry Leaves", ItemType::ITEM, "item/cherry_leaves.png", true, 64);
    itemsById[cherry_leaves->itemId] = cherry_leaves.get();
    items["cherry_leaves"] = std::move(cherry_leaves);
    
    auto jungle_leaves = std::make_unique<Item>(itemId++, "Jungle Leaves", ItemType::ITEM, "item/jungle_leaves.png", true, 64);
    itemsById[jungle_leaves->itemId] = jungle_leaves.get();
    items["jungle_leaves"] = std::move(jungle_leaves);
    
    auto mangrove_leaves = std::make_unique<Item>(itemId++, "Mangrove Leaves", ItemType::ITEM, "item/mangrove_leaves.png", true, 64);
    itemsById[mangrove_leaves->itemId] = mangrove_leaves.get();
    items["mangrove_leaves"] = std::move(mangrove_leaves);
    
    auto spruce_leaves = std::make_unique<Item>(itemId++, "Spruce Leaves", ItemType::ITEM, "item/spruce_leaves.png", true, 64);
    itemsById[spruce_leaves->itemId] = spruce_leaves.get();
    items["spruce_leaves"] = std::move(spruce_leaves);
    
    // Planks
    auto acacia_planks = std::make_unique<Item>(itemId++, "Acacia Planks", ItemType::ITEM, "item/acacia_planks.png", true, 64);
    itemsById[acacia_planks->itemId] = acacia_planks.get();
    items["acacia_planks"] = std::move(acacia_planks);
    
    auto birch_planks = std::make_unique<Item>(itemId++, "Birch Planks", ItemType::ITEM, "item/birch_planks.png", true, 64);
    itemsById[birch_planks->itemId] = birch_planks.get();
    items["birch_planks"] = std::move(birch_planks);
    
    auto cherry_planks = std::make_unique<Item>(itemId++, "Cherry Planks", ItemType::ITEM, "item/cherry_planks.png", true, 64);
    itemsById[cherry_planks->itemId] = cherry_planks.get();
    items["cherry_planks"] = std::move(cherry_planks);
    
    auto jungle_planks = std::make_unique<Item>(itemId++, "Jungle Planks", ItemType::ITEM, "item/jungle_planks.png", true, 64);
    itemsById[jungle_planks->itemId] = jungle_planks.get();
    items["jungle_planks"] = std::move(jungle_planks);
    
    auto spruce_planks = std::make_unique<Item>(itemId++, "Spruce Planks", ItemType::ITEM, "item/spruce_planks.png", true, 64);
    itemsById[spruce_planks->itemId] = spruce_planks.get();
    items["spruce_planks"] = std::move(spruce_planks);
    
    // Common blocks
    auto andesite = std::make_unique<Item>(itemId++, "Andesite", ItemType::ITEM, "item/andesite.png", true, 64);
    itemsById[andesite->itemId] = andesite.get();
    items["andesite"] = std::move(andesite);
    
    auto granite = std::make_unique<Item>(itemId++, "Granite", ItemType::ITEM, "item/granite.png", true, 64);
    itemsById[granite->itemId] = granite.get();
    items["granite"] = std::move(granite);
    
    auto gravel = std::make_unique<Item>(itemId++, "Gravel", ItemType::ITEM, "item/gravel.png", true, 64);
    itemsById[gravel->itemId] = gravel.get();
    items["gravel"] = std::move(gravel);
    
    auto sand = std::make_unique<Item>(itemId++, "Sand", ItemType::ITEM, "item/sand.png", true, 64);
    itemsById[sand->itemId] = sand.get();
    items["sand"] = std::move(sand);
    
    auto glass = std::make_unique<Item>(itemId++, "Glass", ItemType::ITEM, "item/glass.png", true, 64);
    itemsById[glass->itemId] = glass.get();
    items["glass"] = std::move(glass);
    
    auto obsidian = std::make_unique<Item>(itemId++, "Obsidian", ItemType::ITEM, "item/obsidian.png", true, 64);
    itemsById[obsidian->itemId] = obsidian.get();
    items["obsidian"] = std::move(obsidian);
    
    auto bedrock = std::make_unique<Item>(itemId++, "Bedrock", ItemType::ITEM, "item/bedrock.png", true, 64);
    itemsById[bedrock->itemId] = bedrock.get();
    items["bedrock"] = std::move(bedrock);
    
    // Ores
    auto copper_ore = std::make_unique<Item>(itemId++, "Copper Ore", ItemType::ITEM, "item/copper_ore.png", true, 64);
    itemsById[copper_ore->itemId] = copper_ore.get();
    items["copper_ore"] = std::move(copper_ore);
    
    auto iron_ore = std::make_unique<Item>(itemId++, "Iron Ore", ItemType::ITEM, "item/iron_ore.png", true, 64);
    itemsById[iron_ore->itemId] = iron_ore.get();
    items["iron_ore"] = std::move(iron_ore);
    
    auto gold_ore = std::make_unique<Item>(itemId++, "Gold Ore", ItemType::ITEM, "item/gold_ore.png", true, 64);
    itemsById[gold_ore->itemId] = gold_ore.get();
    items["gold_ore"] = std::move(gold_ore);
    
    auto emerald_ore = std::make_unique<Item>(itemId++, "Emerald Ore", ItemType::ITEM, "item/emerald_ore.png", true, 64);
    itemsById[emerald_ore->itemId] = emerald_ore.get();
    items["emerald_ore"] = std::move(emerald_ore);
    
    // Blocks
    auto copper_block = std::make_unique<Item>(itemId++, "Copper Block", ItemType::ITEM, "item/copper_block.png", true, 64);
    itemsById[copper_block->itemId] = copper_block.get();
    items["copper_block"] = std::move(copper_block);
    
    auto iron_block = std::make_unique<Item>(itemId++, "Iron Block", ItemType::ITEM, "item/iron_block.png", true, 64);
    itemsById[iron_block->itemId] = iron_block.get();
    items["iron_block"] = std::move(iron_block);
    
    auto gold_block = std::make_unique<Item>(itemId++, "Gold Block", ItemType::ITEM, "item/gold_block.png", true, 64);
    itemsById[gold_block->itemId] = gold_block.get();
    items["gold_block"] = std::move(gold_block);
    
    auto emerald_block = std::make_unique<Item>(itemId++, "Emerald Block", ItemType::ITEM, "item/emerald_block.png", true, 64);
    itemsById[emerald_block->itemId] = emerald_block.get();
    items["emerald_block"] = std::move(emerald_block);
    
    // Wool blocks
    auto blue_wool = std::make_unique<Item>(itemId++, "Blue Wool", ItemType::ITEM, "item/blue_wool.png", true, 64);
    itemsById[blue_wool->itemId] = blue_wool.get();
    items["blue_wool"] = std::move(blue_wool);
    
    auto gray_wool = std::make_unique<Item>(itemId++, "Gray Wool", ItemType::ITEM, "item/gray_wool.png", true, 64);
    itemsById[gray_wool->itemId] = gray_wool.get();
    items["gray_wool"] = std::move(gray_wool);
    
    auto green_wool = std::make_unique<Item>(itemId++, "Green Wool", ItemType::ITEM, "item/green_wool.png", true, 64);
    itemsById[green_wool->itemId] = green_wool.get();
    items["green_wool"] = std::move(green_wool);
    
    auto light_blue_wool = std::make_unique<Item>(itemId++, "Light Blue Wool", ItemType::ITEM, "item/light_blue_wool.png", true, 64);
    itemsById[light_blue_wool->itemId] = light_blue_wool.get();
    items["light_blue_wool"] = std::move(light_blue_wool);
    
    auto light_gray_wool = std::make_unique<Item>(itemId++, "Light Gray Wool", ItemType::ITEM, "item/light_gray_wool.png", true, 64);
    itemsById[light_gray_wool->itemId] = light_gray_wool.get();
    items["light_gray_wool"] = std::move(light_gray_wool);
    
    auto orange_wool = std::make_unique<Item>(itemId++, "Orange Wool", ItemType::ITEM, "item/orange_wool.png", true, 64);
    itemsById[orange_wool->itemId] = orange_wool.get();
    items["orange_wool"] = std::move(orange_wool);
    
    auto pink_wool = std::make_unique<Item>(itemId++, "Pink Wool", ItemType::ITEM, "item/pink_wool.png", true, 64);
    itemsById[pink_wool->itemId] = pink_wool.get();
    items["pink_wool"] = std::move(pink_wool);
    
    auto purple_wool = std::make_unique<Item>(itemId++, "Purple Wool", ItemType::ITEM, "item/purple_wool.png", true, 64);
    itemsById[purple_wool->itemId] = purple_wool.get();
    items["purple_wool"] = std::move(purple_wool);
    
    auto yellow_wool = std::make_unique<Item>(itemId++, "Yellow Wool", ItemType::ITEM, "item/yellow_wool.png", true, 64);
    itemsById[yellow_wool->itemId] = yellow_wool.get();
    items["yellow_wool"] = std::move(yellow_wool);
    
    // Misc blocks
    auto bricks = std::make_unique<Item>(itemId++, "Bricks", ItemType::ITEM, "item/bricks.png", true, 64);
    itemsById[bricks->itemId] = bricks.get();
    items["bricks"] = std::move(bricks);
    
    auto stone_bricks = std::make_unique<Item>(itemId++, "Stone Bricks", ItemType::ITEM, "item/stone_bricks.png", true, 64);
    itemsById[stone_bricks->itemId] = stone_bricks.get();
    items["stone_bricks"] = std::move(stone_bricks);
    
    auto glowstone = std::make_unique<Item>(itemId++, "Glowstone", ItemType::ITEM, "item/glowstone.png", true, 64);
    itemsById[glowstone->itemId] = glowstone.get();
    items["glowstone"] = std::move(glowstone);
    
    auto ice = std::make_unique<Item>(itemId++, "Ice", ItemType::ITEM, "item/ice.png", true, 64);
    itemsById[ice->itemId] = ice.get();
    items["ice"] = std::move(ice);
    
    auto snow = std::make_unique<Item>(itemId++, "Snow", ItemType::ITEM, "item/snow.png", true, 64);
    itemsById[snow->itemId] = snow.get();
    items["snow"] = std::move(snow);
    
    DEBUG_INVENTORY("Initialized " << (itemId - 100) << " block items and " << items.size() << " total items");
}