import csv
import json

def convert_csv_to_blocks():
    blocks = {}
    
    # Start block IDs after the existing ones (AIR=0, STONE=1, DIRT=2, GRASS=3)
    next_block_id = 4
    
    with open('all_texture_blocks.csv', 'r') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            block_key = row['block_key']
            block_name = row['block_name']
            texture_file = row['texture_file']
            
            # Skip empty rows
            if not block_key or not block_name or not texture_file:
                continue
                
            blocks[block_key] = {
                "blockType": next_block_id,
                "blockName": block_name,
                "textures": {
                    "all": texture_file
                }
            }
            next_block_id += 1
    
    # Create the complete block configuration
    block_config = {
        "blocks": blocks,
        "metadata": {
            "total_blocks": len(blocks),
            "id_range": f"4-{next_block_id-1}",
            "description": "Single-texture blocks from CSV"
        }
    }
    
    with open('blocks_config.json', 'w') as jsonfile:
        json.dump(block_config, jsonfile, indent=2)
    
    print(f"Converted {len(blocks)} blocks to blocks_config.json")
    print(f"Block ID range: 4-{next_block_id-1}")
    return next_block_id

if __name__ == "__main__":
    convert_csv_to_blocks()
