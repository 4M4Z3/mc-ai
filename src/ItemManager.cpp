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
            
        // Saplings
        case BlockType::ACACIA_SAPLING:
            return getItem("acacia_sapling");
        case BlockType::BIRCH_SAPLING:
            return getItem("birch_sapling");
        case BlockType::CHERRY_SAPLING:
            return getItem("cherry_sapling");
        case BlockType::DARK_OAK_SAPLING:
            return getItem("dark_oak_sapling");
        case BlockType::JUNGLE_SAPLING:
            return getItem("jungle_sapling");
        case BlockType::SPRUCE_SAPLING:
            return getItem("spruce_sapling");
            
        // Common blocks
        case BlockType::ANDESITE:
            return getItem("andesite");
        case BlockType::GRANITE:
            return getItem("granite");
        case BlockType::GRAVEL:
            return getItem("gravel");
        case BlockType::SAND:
            return getItem("sand");
        case BlockType::RED_SAND:
            return getItem("red_sand");
        case BlockType::GLASS:
            return getItem("glass");
        case BlockType::OBSIDIAN:
            return getItem("obsidian");
        case BlockType::BEDROCK:
            return getItem("bedrock");
        case BlockType::TINTED_GLASS:
            return getItem("tinted_glass");
        case BlockType::SMOOTH_STONE:
            return getItem("smooth_stone");
        case BlockType::SMOOTH_BASALT:
            return getItem("smooth_basalt");
        case BlockType::DRIPSTONE_BLOCK:
            return getItem("dripstone_block");
        case BlockType::AMETHYST_BLOCK:
            return getItem("amethyst_block");
        case BlockType::AMETHYST_CLUSTER:
            return getItem("amethyst_cluster");
            
        // Ores
        case BlockType::COPPER_ORE:
            return getItem("copper_ore");
        case BlockType::IRON_ORE:
            return getItem("iron_ore");
        case BlockType::GOLD_ORE:
            return getItem("gold_ore");
        case BlockType::EMERALD_ORE:
            return getItem("emerald_ore");
            
        // Metal blocks
        case BlockType::COPPER_BLOCK:
            return getItem("copper_block");
        case BlockType::IRON_BLOCK:
            return getItem("iron_block");
        case BlockType::GOLD_BLOCK:
            return getItem("gold_block");
        case BlockType::EMERALD_BLOCK:
            return getItem("emerald_block");
        case BlockType::RAW_COPPER_BLOCK:
            return getItem("raw_copper_block");
        case BlockType::RAW_IRON_BLOCK:
            return getItem("raw_iron_block");
        case BlockType::RAW_GOLD_BLOCK:
            return getItem("raw_gold_block");
            
        // Copper variants
        case BlockType::CHISELED_COPPER:
            return getItem("chiseled_copper");
        case BlockType::OXIDIZED_COPPER:
            return getItem("oxidized_copper");
        case BlockType::OXIDIZED_CHISELED_COPPER:
            return getItem("oxidized_chiseled_copper");
        case BlockType::COPPER_GRATE:
            return getItem("copper_grate");
        case BlockType::COPPER_BULB:
            return getItem("copper_bulb");
        case BlockType::COPPER_BULB_LIT:
            return getItem("copper_bulb"); // Same item as unlit
        case BlockType::COPPER_BULB_POWERED:
            return getItem("copper_bulb"); // Same item as unlit
        case BlockType::COPPER_BULB_LIT_POWERED:
            return getItem("copper_bulb"); // Same item as unlit
        case BlockType::COPPER_TRAPDOOR:
            return getItem("copper_trapdoor");
            
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
            
        // Stained glass
        case BlockType::BLUE_STAINED_GLASS:
            return getItem("blue_stained_glass");
        case BlockType::BROWN_STAINED_GLASS:
            return getItem("brown_stained_glass");
        case BlockType::CYAN_STAINED_GLASS:
            return getItem("cyan_stained_glass");
        case BlockType::GRAY_STAINED_GLASS:
            return getItem("gray_stained_glass");
        case BlockType::GREEN_STAINED_GLASS:
            return getItem("green_stained_glass");
        case BlockType::LIGHT_BLUE_STAINED_GLASS:
            return getItem("light_blue_stained_glass");
        case BlockType::LIGHT_GRAY_STAINED_GLASS:
            return getItem("light_gray_stained_glass");
        case BlockType::LIME_STAINED_GLASS:
            return getItem("lime_stained_glass");
        case BlockType::MAGENTA_STAINED_GLASS:
            return getItem("magenta_stained_glass");
        case BlockType::ORANGE_STAINED_GLASS:
            return getItem("orange_stained_glass");
        case BlockType::PINK_STAINED_GLASS:
            return getItem("pink_stained_glass");
        case BlockType::YELLOW_STAINED_GLASS:
            return getItem("yellow_stained_glass");
            
        // Terracotta
        case BlockType::TERRACOTTA:
            return getItem("terracotta");
        case BlockType::BLUE_TERRACOTTA:
            return getItem("blue_terracotta");
        case BlockType::GRAY_TERRACOTTA:
            return getItem("gray_terracotta");
        case BlockType::GREEN_TERRACOTTA:
            return getItem("green_terracotta");
        case BlockType::LIGHT_BLUE_TERRACOTTA:
            return getItem("light_blue_terracotta");
        case BlockType::LIGHT_GRAY_TERRACOTTA:
            return getItem("light_gray_terracotta");
        case BlockType::ORANGE_TERRACOTTA:
            return getItem("orange_terracotta");
        case BlockType::PINK_TERRACOTTA:
            return getItem("pink_terracotta");
        case BlockType::YELLOW_TERRACOTTA:
            return getItem("yellow_terracotta");
            
        // Concrete
        case BlockType::BROWN_CONCRETE:
            return getItem("brown_concrete");
        case BlockType::GRAY_CONCRETE:
            return getItem("gray_concrete");
        case BlockType::GREEN_CONCRETE:
            return getItem("green_concrete");
        case BlockType::LIGHT_GRAY_CONCRETE:
            return getItem("light_gray_concrete");
        case BlockType::LIME_CONCRETE:
            return getItem("lime_concrete");
        case BlockType::ORANGE_CONCRETE:
            return getItem("orange_concrete");
        case BlockType::PINK_CONCRETE:
            return getItem("pink_concrete");
        case BlockType::RED_CONCRETE:
            return getItem("red_concrete");
            
        // Concrete powder
        case BlockType::BROWN_CONCRETE_POWDER:
            return getItem("brown_concrete_powder");
        case BlockType::GRAY_CONCRETE_POWDER:
            return getItem("gray_concrete_powder");
        case BlockType::GREEN_CONCRETE_POWDER:
            return getItem("green_concrete_powder");
        case BlockType::LIGHT_GRAY_CONCRETE_POWDER:
            return getItem("light_gray_concrete_powder");
        case BlockType::LIME_CONCRETE_POWDER:
            return getItem("lime_concrete_powder");
        case BlockType::ORANGE_CONCRETE_POWDER:
            return getItem("orange_concrete_powder");
        case BlockType::PINK_CONCRETE_POWDER:
            return getItem("pink_concrete_powder");
        case BlockType::RED_CONCRETE_POWDER:
            return getItem("red_concrete_powder");
            
        // Glazed terracotta
        case BlockType::BROWN_GLAZED_TERRACOTTA:
            return getItem("brown_glazed_terracotta");
        case BlockType::GRAY_GLAZED_TERRACOTTA:
            return getItem("gray_glazed_terracotta");
        case BlockType::GREEN_GLAZED_TERRACOTTA:
            return getItem("green_glazed_terracotta");
        case BlockType::LIGHT_GRAY_GLAZED_TERRACOTTA:
            return getItem("light_gray_glazed_terracotta");
        case BlockType::LIME_GLAZED_TERRACOTTA:
            return getItem("lime_glazed_terracotta");
        case BlockType::ORANGE_GLAZED_TERRACOTTA:
            return getItem("orange_glazed_terracotta");
        case BlockType::PINK_GLAZED_TERRACOTTA:
            return getItem("pink_glazed_terracotta");
        case BlockType::RED_GLAZED_TERRACOTTA:
            return getItem("red_glazed_terracotta");
            
        // Stone variants
        case BlockType::BRICKS:
            return getItem("bricks");
        case BlockType::STONE_BRICKS:
            return getItem("stone_bricks");
        case BlockType::CHISELED_STONE_BRICKS:
            return getItem("chiseled_stone_bricks");
        case BlockType::CRACKED_STONE_BRICKS:
            return getItem("cracked_stone_bricks");
        case BlockType::CHISELED_SANDSTONE:
            return getItem("chiseled_sandstone");
        case BlockType::CHISELED_RED_SANDSTONE:
            return getItem("chiseled_red_sandstone");
        case BlockType::POLISHED_GRANITE:
            return getItem("polished_granite");
        case BlockType::POLISHED_DIORITE:
            return getItem("polished_diorite");
        case BlockType::POLISHED_BLACKSTONE:
            return getItem("polished_blackstone");
        case BlockType::POLISHED_BLACKSTONE_BRICKS:
            return getItem("polished_blackstone_bricks");
        case BlockType::CHISELED_POLISHED_BLACKSTONE:
            return getItem("chiseled_polished_blackstone");
        case BlockType::CRACKED_POLISHED_BLACKSTONE_BRICKS:
            return getItem("cracked_polished_blackstone_bricks");
        case BlockType::CHISELED_DEEPSLATE:
            return getItem("chiseled_deepslate");
        case BlockType::POLISHED_DEEPSLATE:
            return getItem("polished_deepslate");
        case BlockType::CRACKED_DEEPSLATE_BRICKS:
            return getItem("cracked_deepslate_bricks");
        case BlockType::CRACKED_DEEPSLATE_TILES:
            return getItem("cracked_deepslate_tiles");
        case BlockType::POLISHED_TUFF:
            return getItem("polished_tuff");
        case BlockType::TUFF:
            return getItem("tuff");
        case BlockType::TUFF_BRICKS:
            return getItem("tuff_bricks");
            
        // Special blocks
        case BlockType::GLOWSTONE:
            return getItem("glowstone");
        case BlockType::ICE:
            return getItem("ice");
        case BlockType::FROSTED_ICE:
            return getItem("ice"); // Frosted ice drops regular ice
        case BlockType::SNOW:
            return getItem("snow");
        case BlockType::POWDER_SNOW:
            return getItem("powder_snow");
        case BlockType::SEA_LANTERN:
            return getItem("sea_lantern");
        case BlockType::LANTERN:
            return getItem("lantern");
        case BlockType::SOUL_LANTERN:
            return getItem("soul_lantern");
        case BlockType::SOUL_TORCH:
            return getItem("soul_torch");
        case BlockType::SOUL_SAND:
            return getItem("soul_sand");
        case BlockType::SOUL_SOIL:
            return getItem("soul_soil");
        case BlockType::BEACON:
            return getItem("beacon");
        case BlockType::CONDUIT:
            return getItem("conduit");
        case BlockType::SPAWNER:
            return getItem("spawner");
        case BlockType::SPONGE:
            return getItem("sponge");
        case BlockType::SLIME_BLOCK:
            return getItem("slime_block");
        case BlockType::HONEYCOMB_BLOCK:
            return getItem("honeycomb_block");
        case BlockType::SHROOMLIGHT:
            return getItem("shroomlight");
        case BlockType::JACK_O_LANTERN:
            return getItem("jack_o_lantern");
        case BlockType::HEAVY_CORE:
            return getItem("heavy_core");
        case BlockType::PURPUR_BLOCK:
            return getItem("purpur_block");
        case BlockType::QUARTZ_BRICKS:
            return getItem("quartz_bricks");
            
        // Nether blocks
        case BlockType::CHISELED_NETHER_BRICKS:
            return getItem("chiseled_nether_bricks");
        case BlockType::CRACKED_NETHER_BRICKS:
            return getItem("cracked_nether_bricks");
        case BlockType::RED_NETHER_BRICKS:
            return getItem("red_nether_bricks");
            
        // Mushroom blocks
        case BlockType::BROWN_MUSHROOM_BLOCK:
            return getItem("brown_mushroom_block");
        case BlockType::RED_MUSHROOM:
            return getItem("red_mushroom");
        case BlockType::RED_MUSHROOM_BLOCK:
            return getItem("red_mushroom_block");
            
        // Coral blocks
        case BlockType::BRAIN_CORAL:
            return getItem("brain_coral");
        case BlockType::BRAIN_CORAL_BLOCK:
            return getItem("brain_coral_block");
        case BlockType::HORN_CORAL:
            return getItem("horn_coral");
        case BlockType::HORN_CORAL_BLOCK:
            return getItem("horn_coral_block");
        case BlockType::HORN_CORAL_FAN:
            return getItem("horn_coral_fan");
        case BlockType::TUBE_CORAL:
            return getItem("tube_coral");
        case BlockType::TUBE_CORAL_BLOCK:
            return getItem("tube_coral_block");
        case BlockType::TUBE_CORAL_FAN:
            return getItem("tube_coral_fan");
        case BlockType::DEAD_BRAIN_CORAL_FAN:
            return getItem("dead_brain_coral_fan");
        case BlockType::DEAD_BUBBLE_CORAL_FAN:
            return getItem("dead_bubble_coral_fan");
        case BlockType::DEAD_FIRE_CORAL_FAN:
            return getItem("dead_fire_coral_fan");
            
        // Plant blocks
        case BlockType::ALLIUM:
            return getItem("allium");
        case BlockType::AZURE_BLUET:
            return getItem("azure_bluet");
        case BlockType::DANDELION:
            return getItem("dandelion");
        case BlockType::DEAD_BUSH:
            return getItem("dead_bush");
        case BlockType::AZALEA_PLANT:
            return getItem("azalea_plant");
        case BlockType::SPORE_BLOSSOM:
            return getItem("spore_blossom");
        case BlockType::SPORE_BLOSSOM_BASE:
            return getItem("spore_blossom_base");
        case BlockType::PINK_PETALS:
            return getItem("pink_petals");
        case BlockType::PINK_PETALS_STEM:
            return getItem("pink_petals_stem");
        case BlockType::TWISTING_VINES:
            return getItem("twisting_vines");
        case BlockType::TWISTING_VINES_PLANT:
            return getItem("twisting_vines_plant");
        case BlockType::KELP:
            return getItem("kelp");
        case BlockType::KELP_META:
            return getItem("kelp"); // Same as regular kelp
            
        // Misc utility blocks
        case BlockType::MOSS_BLOCK:
            return getItem("moss_block");
        case BlockType::MUD:
            return getItem("mud");
        case BlockType::ROOTED_DIRT:
            return getItem("rooted_dirt");
        case BlockType::ITEM_FRAME:
            return getItem("item_frame");
        case BlockType::RAIL:
            return getItem("rail");
        case BlockType::RAIL_CORNER:
            return getItem("rail"); // Same as regular rail
        case BlockType::TRIPWIRE:
            return getItem("tripwire");
        case BlockType::TRIPWIRE_HOOK:
            return getItem("tripwire_hook");
        case BlockType::SPRUCE_TRAPDOOR:
            return getItem("spruce_trapdoor");
            
        // Resin blocks
        case BlockType::RESIN_BLOCK:
            return getItem("resin_block");
        case BlockType::RESIN_BRICKS:
            return getItem("resin_bricks");
        case BlockType::CHISELED_RESIN_BRICKS:
            return getItem("chiseled_resin_bricks");
        case BlockType::RESIN_CLUMP:
            return getItem("resin_clump");
            
        // Shulker boxes
        case BlockType::GRAY_SHULKER_BOX:
            return getItem("gray_shulker_box");
            
        // Candles
        case BlockType::BLACK_CANDLE:
            return getItem("black_candle");
        case BlockType::BLACK_CANDLE_LIT:
            return getItem("black_candle"); // Same item as unlit
        case BlockType::BLUE_CANDLE:
            return getItem("blue_candle");
        case BlockType::BLUE_CANDLE_LIT:
            return getItem("blue_candle"); // Same item as unlit
        case BlockType::RED_CANDLE:
            return getItem("red_candle");
        case BlockType::RED_CANDLE_LIT:
            return getItem("red_candle"); // Same item as unlit
        case BlockType::WHITE_CANDLE:
            return getItem("white_candle");
        case BlockType::WHITE_CANDLE_LIT:
            return getItem("white_candle"); // Same item as unlit
        case BlockType::YELLOW_CANDLE:
            return getItem("yellow_candle");
        case BlockType::YELLOW_CANDLE_LIT:
            return getItem("yellow_candle"); // Same item as unlit
            
        // Crop stages - these typically don't drop themselves
        case BlockType::WHEAT_STAGE0:
        case BlockType::WHEAT_STAGE1:
        case BlockType::WHEAT_STAGE_2:
        case BlockType::WHEAT_STAGE_3:
        case BlockType::WHEAT_STAGE_4:
        case BlockType::WHEAT_STAGE_5:
        case BlockType::WHEAT_STAGE_6:
        case BlockType::WHEAT_STAGE_7:
            return getItem("wheat_seeds"); // Wheat crops drop seeds (except final stage which drops wheat)
        case BlockType::POTATOES_STAGE3:
            return getItem("potato");
            
        // Destroy stages - these are visual only, shouldn't be collectable
        case BlockType::DESTROY_STAGE_7:
        case BlockType::DESTROY_STAGE_8:
        case BlockType::DESTROY_STAGE_9:
            return nullptr;
            
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
    
    // Tools and materials
    auto stick = std::make_unique<Item>(itemId++, "Stick", ItemType::ITEM, "item/stick.png", true, 64);
    itemsById[stick->itemId] = stick.get();
    items["stick"] = std::move(stick);
    
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
    
    // ========== ADDITIONAL COMPREHENSIVE ITEMS ==========
    
    // Saplings
    auto acacia_sapling = std::make_unique<Item>(itemId++, "Acacia Sapling", ItemType::ITEM, "item/acacia_sapling.png", true, 64);
    itemsById[acacia_sapling->itemId] = acacia_sapling.get();
    items["acacia_sapling"] = std::move(acacia_sapling);
    
    auto birch_sapling = std::make_unique<Item>(itemId++, "Birch Sapling", ItemType::ITEM, "item/birch_sapling.png", true, 64);
    itemsById[birch_sapling->itemId] = birch_sapling.get();
    items["birch_sapling"] = std::move(birch_sapling);
    
    auto cherry_sapling = std::make_unique<Item>(itemId++, "Cherry Sapling", ItemType::ITEM, "item/cherry_sapling.png", true, 64);
    itemsById[cherry_sapling->itemId] = cherry_sapling.get();
    items["cherry_sapling"] = std::move(cherry_sapling);
    
    auto dark_oak_sapling = std::make_unique<Item>(itemId++, "Dark Oak Sapling", ItemType::ITEM, "item/dark_oak_sapling.png", true, 64);
    itemsById[dark_oak_sapling->itemId] = dark_oak_sapling.get();
    items["dark_oak_sapling"] = std::move(dark_oak_sapling);
    
    auto jungle_sapling = std::make_unique<Item>(itemId++, "Jungle Sapling", ItemType::ITEM, "item/jungle_sapling.png", true, 64);
    itemsById[jungle_sapling->itemId] = jungle_sapling.get();
    items["jungle_sapling"] = std::move(jungle_sapling);
    
    auto spruce_sapling = std::make_unique<Item>(itemId++, "Spruce Sapling", ItemType::ITEM, "item/spruce_sapling.png", true, 64);
    itemsById[spruce_sapling->itemId] = spruce_sapling.get();
    items["spruce_sapling"] = std::move(spruce_sapling);
    
    // Additional sand and stone types
    auto red_sand = std::make_unique<Item>(itemId++, "Red Sand", ItemType::ITEM, "item/red_sand.png", true, 64);
    itemsById[red_sand->itemId] = red_sand.get();
    items["red_sand"] = std::move(red_sand);
    
    auto tinted_glass = std::make_unique<Item>(itemId++, "Tinted Glass", ItemType::ITEM, "item/tinted_glass.png", true, 64);
    itemsById[tinted_glass->itemId] = tinted_glass.get();
    items["tinted_glass"] = std::move(tinted_glass);
    
    auto smooth_stone = std::make_unique<Item>(itemId++, "Smooth Stone", ItemType::ITEM, "item/smooth_stone.png", true, 64);
    itemsById[smooth_stone->itemId] = smooth_stone.get();
    items["smooth_stone"] = std::move(smooth_stone);
    
    auto smooth_basalt = std::make_unique<Item>(itemId++, "Smooth Basalt", ItemType::ITEM, "item/smooth_basalt.png", true, 64);
    itemsById[smooth_basalt->itemId] = smooth_basalt.get();
    items["smooth_basalt"] = std::move(smooth_basalt);
    
    auto dripstone_block = std::make_unique<Item>(itemId++, "Dripstone Block", ItemType::ITEM, "item/dripstone_block.png", true, 64);
    itemsById[dripstone_block->itemId] = dripstone_block.get();
    items["dripstone_block"] = std::move(dripstone_block);
    
    auto amethyst_block = std::make_unique<Item>(itemId++, "Amethyst Block", ItemType::ITEM, "item/amethyst_block.png", true, 64);
    itemsById[amethyst_block->itemId] = amethyst_block.get();
    items["amethyst_block"] = std::move(amethyst_block);
    
    auto amethyst_cluster = std::make_unique<Item>(itemId++, "Amethyst Cluster", ItemType::ITEM, "item/amethyst_cluster.png", true, 64);
    itemsById[amethyst_cluster->itemId] = amethyst_cluster.get();
    items["amethyst_cluster"] = std::move(amethyst_cluster);
    
    // Raw metal blocks
    auto raw_copper_block = std::make_unique<Item>(itemId++, "Raw Copper Block", ItemType::ITEM, "item/raw_copper_block.png", true, 64);
    itemsById[raw_copper_block->itemId] = raw_copper_block.get();
    items["raw_copper_block"] = std::move(raw_copper_block);
    
    auto raw_iron_block = std::make_unique<Item>(itemId++, "Raw Iron Block", ItemType::ITEM, "item/raw_iron_block.png", true, 64);
    itemsById[raw_iron_block->itemId] = raw_iron_block.get();
    items["raw_iron_block"] = std::move(raw_iron_block);
    
    auto raw_gold_block = std::make_unique<Item>(itemId++, "Raw Gold Block", ItemType::ITEM, "item/raw_gold_block.png", true, 64);
    itemsById[raw_gold_block->itemId] = raw_gold_block.get();
    items["raw_gold_block"] = std::move(raw_gold_block);
    
    // Copper variants
    auto chiseled_copper = std::make_unique<Item>(itemId++, "Chiseled Copper", ItemType::ITEM, "item/chiseled_copper.png", true, 64);
    itemsById[chiseled_copper->itemId] = chiseled_copper.get();
    items["chiseled_copper"] = std::move(chiseled_copper);
    
    auto oxidized_copper = std::make_unique<Item>(itemId++, "Oxidized Copper", ItemType::ITEM, "item/oxidized_copper.png", true, 64);
    itemsById[oxidized_copper->itemId] = oxidized_copper.get();
    items["oxidized_copper"] = std::move(oxidized_copper);
    
    auto oxidized_chiseled_copper = std::make_unique<Item>(itemId++, "Oxidized Chiseled Copper", ItemType::ITEM, "item/oxidized_chiseled_copper.png", true, 64);
    itemsById[oxidized_chiseled_copper->itemId] = oxidized_chiseled_copper.get();
    items["oxidized_chiseled_copper"] = std::move(oxidized_chiseled_copper);
    
    auto copper_grate = std::make_unique<Item>(itemId++, "Copper Grate", ItemType::ITEM, "item/copper_grate.png", true, 64);
    itemsById[copper_grate->itemId] = copper_grate.get();
    items["copper_grate"] = std::move(copper_grate);
    
    auto copper_bulb = std::make_unique<Item>(itemId++, "Copper Bulb", ItemType::ITEM, "item/copper_bulb.png", true, 64);
    itemsById[copper_bulb->itemId] = copper_bulb.get();
    items["copper_bulb"] = std::move(copper_bulb);
    
    auto copper_trapdoor = std::make_unique<Item>(itemId++, "Copper Trapdoor", ItemType::ITEM, "item/copper_trapdoor.png", true, 64);
    itemsById[copper_trapdoor->itemId] = copper_trapdoor.get();
    items["copper_trapdoor"] = std::move(copper_trapdoor);
    
    // Stained glass
    auto blue_stained_glass = std::make_unique<Item>(itemId++, "Blue Stained Glass", ItemType::ITEM, "item/blue_stained_glass.png", true, 64);
    itemsById[blue_stained_glass->itemId] = blue_stained_glass.get();
    items["blue_stained_glass"] = std::move(blue_stained_glass);
    
    auto brown_stained_glass = std::make_unique<Item>(itemId++, "Brown Stained Glass", ItemType::ITEM, "item/brown_stained_glass.png", true, 64);
    itemsById[brown_stained_glass->itemId] = brown_stained_glass.get();
    items["brown_stained_glass"] = std::move(brown_stained_glass);
    
    auto cyan_stained_glass = std::make_unique<Item>(itemId++, "Cyan Stained Glass", ItemType::ITEM, "item/cyan_stained_glass.png", true, 64);
    itemsById[cyan_stained_glass->itemId] = cyan_stained_glass.get();
    items["cyan_stained_glass"] = std::move(cyan_stained_glass);
    
    auto gray_stained_glass = std::make_unique<Item>(itemId++, "Gray Stained Glass", ItemType::ITEM, "item/gray_stained_glass.png", true, 64);
    itemsById[gray_stained_glass->itemId] = gray_stained_glass.get();
    items["gray_stained_glass"] = std::move(gray_stained_glass);
    
    auto green_stained_glass = std::make_unique<Item>(itemId++, "Green Stained Glass", ItemType::ITEM, "item/green_stained_glass.png", true, 64);
    itemsById[green_stained_glass->itemId] = green_stained_glass.get();
    items["green_stained_glass"] = std::move(green_stained_glass);
    
    auto light_blue_stained_glass = std::make_unique<Item>(itemId++, "Light Blue Stained Glass", ItemType::ITEM, "item/light_blue_stained_glass.png", true, 64);
    itemsById[light_blue_stained_glass->itemId] = light_blue_stained_glass.get();
    items["light_blue_stained_glass"] = std::move(light_blue_stained_glass);
    
    auto light_gray_stained_glass = std::make_unique<Item>(itemId++, "Light Gray Stained Glass", ItemType::ITEM, "item/light_gray_stained_glass.png", true, 64);
    itemsById[light_gray_stained_glass->itemId] = light_gray_stained_glass.get();
    items["light_gray_stained_glass"] = std::move(light_gray_stained_glass);
    
    auto lime_stained_glass = std::make_unique<Item>(itemId++, "Lime Stained Glass", ItemType::ITEM, "item/lime_stained_glass.png", true, 64);
    itemsById[lime_stained_glass->itemId] = lime_stained_glass.get();
    items["lime_stained_glass"] = std::move(lime_stained_glass);
    
    auto magenta_stained_glass = std::make_unique<Item>(itemId++, "Magenta Stained Glass", ItemType::ITEM, "item/magenta_stained_glass.png", true, 64);
    itemsById[magenta_stained_glass->itemId] = magenta_stained_glass.get();
    items["magenta_stained_glass"] = std::move(magenta_stained_glass);
    
    auto orange_stained_glass = std::make_unique<Item>(itemId++, "Orange Stained Glass", ItemType::ITEM, "item/orange_stained_glass.png", true, 64);
    itemsById[orange_stained_glass->itemId] = orange_stained_glass.get();
    items["orange_stained_glass"] = std::move(orange_stained_glass);
    
    auto pink_stained_glass = std::make_unique<Item>(itemId++, "Pink Stained Glass", ItemType::ITEM, "item/pink_stained_glass.png", true, 64);
    itemsById[pink_stained_glass->itemId] = pink_stained_glass.get();
    items["pink_stained_glass"] = std::move(pink_stained_glass);
    
    auto yellow_stained_glass = std::make_unique<Item>(itemId++, "Yellow Stained Glass", ItemType::ITEM, "item/yellow_stained_glass.png", true, 64);
    itemsById[yellow_stained_glass->itemId] = yellow_stained_glass.get();
    items["yellow_stained_glass"] = std::move(yellow_stained_glass);
    
    // Essential stone variants
    auto chiseled_stone_bricks = std::make_unique<Item>(itemId++, "Chiseled Stone Bricks", ItemType::ITEM, "item/chiseled_stone_bricks.png", true, 64);
    itemsById[chiseled_stone_bricks->itemId] = chiseled_stone_bricks.get();
    items["chiseled_stone_bricks"] = std::move(chiseled_stone_bricks);
    
    auto cracked_stone_bricks = std::make_unique<Item>(itemId++, "Cracked Stone Bricks", ItemType::ITEM, "item/cracked_stone_bricks.png", true, 64);
    itemsById[cracked_stone_bricks->itemId] = cracked_stone_bricks.get();
    items["cracked_stone_bricks"] = std::move(cracked_stone_bricks);
    
    auto polished_granite = std::make_unique<Item>(itemId++, "Polished Granite", ItemType::ITEM, "item/polished_granite.png", true, 64);
    itemsById[polished_granite->itemId] = polished_granite.get();
    items["polished_granite"] = std::move(polished_granite);
    
    auto polished_diorite = std::make_unique<Item>(itemId++, "Polished Diorite", ItemType::ITEM, "item/polished_diorite.png", true, 64);
    itemsById[polished_diorite->itemId] = polished_diorite.get();
    items["polished_diorite"] = std::move(polished_diorite);
    
    auto tuff = std::make_unique<Item>(itemId++, "Tuff", ItemType::ITEM, "item/tuff.png", true, 64);
    itemsById[tuff->itemId] = tuff.get();
    items["tuff"] = std::move(tuff);
    
    // Special utility blocks
    auto sea_lantern = std::make_unique<Item>(itemId++, "Sea Lantern", ItemType::ITEM, "item/sea_lantern.png", true, 64);
    itemsById[sea_lantern->itemId] = sea_lantern.get();
    items["sea_lantern"] = std::move(sea_lantern);
    
    auto lantern = std::make_unique<Item>(itemId++, "Lantern", ItemType::ITEM, "item/lantern.png", true, 64);
    itemsById[lantern->itemId] = lantern.get();
    items["lantern"] = std::move(lantern);
    
    auto soul_lantern = std::make_unique<Item>(itemId++, "Soul Lantern", ItemType::ITEM, "item/soul_lantern.png", true, 64);
    itemsById[soul_lantern->itemId] = soul_lantern.get();
    items["soul_lantern"] = std::move(soul_lantern);
    
    auto soul_torch = std::make_unique<Item>(itemId++, "Soul Torch", ItemType::ITEM, "item/soul_torch.png", true, 64);
    itemsById[soul_torch->itemId] = soul_torch.get();
    items["soul_torch"] = std::move(soul_torch);
    
    auto soul_sand = std::make_unique<Item>(itemId++, "Soul Sand", ItemType::ITEM, "item/soul_sand.png", true, 64);
    itemsById[soul_sand->itemId] = soul_sand.get();
    items["soul_sand"] = std::move(soul_sand);
    
    auto soul_soil = std::make_unique<Item>(itemId++, "Soul Soil", ItemType::ITEM, "item/soul_soil.png", true, 64);
    itemsById[soul_soil->itemId] = soul_soil.get();
    items["soul_soil"] = std::move(soul_soil);
    
    auto beacon = std::make_unique<Item>(itemId++, "Beacon", ItemType::ITEM, "item/beacon.png", true, 64);
    itemsById[beacon->itemId] = beacon.get();
    items["beacon"] = std::move(beacon);
    
    auto conduit = std::make_unique<Item>(itemId++, "Conduit", ItemType::ITEM, "item/conduit.png", true, 64);
    itemsById[conduit->itemId] = conduit.get();
    items["conduit"] = std::move(conduit);
    
    auto spawner = std::make_unique<Item>(itemId++, "Spawner", ItemType::ITEM, "item/spawner.png", true, 1);
    itemsById[spawner->itemId] = spawner.get();
    items["spawner"] = std::move(spawner);
    
    auto sponge = std::make_unique<Item>(itemId++, "Sponge", ItemType::ITEM, "item/sponge.png", true, 64);
    itemsById[sponge->itemId] = sponge.get();
    items["sponge"] = std::move(sponge);
    
    auto slime_block = std::make_unique<Item>(itemId++, "Slime Block", ItemType::ITEM, "item/slime_block.png", true, 64);
    itemsById[slime_block->itemId] = slime_block.get();
    items["slime_block"] = std::move(slime_block);
    
    auto powder_snow = std::make_unique<Item>(itemId++, "Powder Snow", ItemType::ITEM, "item/powder_snow.png", true, 64);
    itemsById[powder_snow->itemId] = powder_snow.get();
    items["powder_snow"] = std::move(powder_snow);
    
    // Agricultural items
    auto wheat_seeds = std::make_unique<Item>(itemId++, "Wheat Seeds", ItemType::ITEM, "item/wheat_seeds.png", true, 64);
    itemsById[wheat_seeds->itemId] = wheat_seeds.get();
    items["wheat_seeds"] = std::move(wheat_seeds);
    
    auto potato = std::make_unique<Item>(itemId++, "Potato", ItemType::ITEM, "item/potato.png", true, 64);
    itemsById[potato->itemId] = potato.get();
    items["potato"] = std::move(potato);
    
    auto kelp = std::make_unique<Item>(itemId++, "Kelp", ItemType::ITEM, "item/kelp.png", true, 64);
    itemsById[kelp->itemId] = kelp.get();
    items["kelp"] = std::move(kelp);
    
    // Basic plant items
    auto dandelion = std::make_unique<Item>(itemId++, "Dandelion", ItemType::ITEM, "item/dandelion.png", true, 64);
    itemsById[dandelion->itemId] = dandelion.get();
    items["dandelion"] = std::move(dandelion);
    
    auto dead_bush = std::make_unique<Item>(itemId++, "Dead Bush", ItemType::ITEM, "item/dead_bush.png", true, 64);
    itemsById[dead_bush->itemId] = dead_bush.get();
    items["dead_bush"] = std::move(dead_bush);
    
    // Essential utility blocks
    auto rail = std::make_unique<Item>(itemId++, "Rail", ItemType::ITEM, "item/rail.png", true, 64);
    itemsById[rail->itemId] = rail.get();
    items["rail"] = std::move(rail);
    
    auto item_frame = std::make_unique<Item>(itemId++, "Item Frame", ItemType::ITEM, "item/item_frame.png", true, 64);
    itemsById[item_frame->itemId] = item_frame.get();
    items["item_frame"] = std::move(item_frame);
    
    auto jack_o_lantern = std::make_unique<Item>(itemId++, "Jack o'Lantern", ItemType::ITEM, "item/jack_o_lantern.png", true, 64);
    itemsById[jack_o_lantern->itemId] = jack_o_lantern.get();
    items["jack_o_lantern"] = std::move(jack_o_lantern);
    
    auto spruce_trapdoor = std::make_unique<Item>(itemId++, "Spruce Trapdoor", ItemType::ITEM, "item/spruce_trapdoor.png", true, 64);
    itemsById[spruce_trapdoor->itemId] = spruce_trapdoor.get();
    items["spruce_trapdoor"] = std::move(spruce_trapdoor);
    
    DEBUG_INVENTORY("Initialized " << (itemId - 100) << " block items and " << items.size() << " total items");
}