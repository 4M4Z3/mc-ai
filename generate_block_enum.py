import json

def generate_block_enum():
    # Load the blocks configuration
    with open('blocks_config.json', 'r') as f:
        config = json.load(f)
    
    blocks = config['blocks']
    
    # Generate the enum
    enum_lines = [
        "enum class BlockType : uint16_t {",
        "    // Core blocks (unchanged for compatibility)", 
        "    AIR = 0,",
        "    STONE = 1,",
        "    DIRT = 2,", 
        "    GRASS = 3,",
        "",
        "    // Generated blocks from CSV"
    ]
    
    # Sort blocks by ID for consistency
    sorted_blocks = sorted(blocks.items(), key=lambda x: x[1]['blockType'])
    
    for block_key, block_data in sorted_blocks:
        block_id = block_data['blockType']
        # Convert to SCREAMING_SNAKE_CASE
        enum_name = block_key.upper().replace('-', '_')
        enum_lines.append(f"    {enum_name} = {block_id},")
    
    enum_lines.append("};")
    
    # Write to a new header file
    with open('include/BlockTypes.h', 'w') as f:
        f.write("#pragma once\n\n")
        f.write("#include <cstdint>\n\n")
        f.write('\n'.join(enum_lines))
        f.write('\n')
    
    print(f"Generated BlockTypes.h with {len(sorted_blocks)} new block types")
    print(f"Block ID range: 4-{max(block_data['blockType'] for block_data in blocks.values())}")

if __name__ == "__main__":
    generate_block_enum()
