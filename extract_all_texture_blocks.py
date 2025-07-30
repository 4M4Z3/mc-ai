#!/usr/bin/env python3
"""
Script to extract blocks that only have "all" textures from the organized block assets JSON
and create a CSV file with them.
"""

import json
import csv
from pathlib import Path

def read_organized_blocks():
    """Read the organized block assets JSON file."""
    json_file = "organized_block_assets_final.json"
    if not Path(json_file).exists():
        raise FileNotFoundError(f"JSON file '{json_file}' not found. Run the organization script first.")
    
    with open(json_file, 'r', encoding='utf-8') as file:
        return json.load(file)

def extract_all_texture_blocks(blocks_data):
    """Extract blocks that only have 'all' textures."""
    all_texture_blocks = []
    
    for block_key, block_info in blocks_data.items():
        if isinstance(block_info, dict) and 'textures' in block_info:
            textures = block_info['textures']
            
            # Check if the block only has "all" texture (and no other texture types)
            if 'all' in textures and len(textures) == 1:
                all_texture_blocks.append({
                    'block_key': block_key,
                    'block_name': block_info.get('block_name', block_key),
                    'texture_file': textures['all'] if isinstance(textures['all'], str) else ', '.join(textures['all'])
                })
    
    return all_texture_blocks

def main():
    """Main function to extract all-texture blocks and create CSV."""
    print("📖 Reading organized block assets...")
    
    try:
        blocks_data = read_organized_blocks()
        print(f"📁 Found {len(blocks_data)} organized blocks")
        
        print("🔍 Extracting blocks with only 'all' textures...")
        all_texture_blocks = extract_all_texture_blocks(blocks_data)
        
        print(f"✨ Found {len(all_texture_blocks)} blocks that use the same texture for all faces")
        
        if not all_texture_blocks:
            print("❌ No blocks found with only 'all' textures")
            return
        
        # Create CSV file
        output_csv = "all_texture_blocks.csv"
        
        with open(output_csv, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['block_key', 'block_name', 'texture_file']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            # Write header
            writer.writeheader()
            
            # Write data
            for block in all_texture_blocks:
                writer.writerow(block)
        
        print(f"🎉 Successfully created '{output_csv}' with {len(all_texture_blocks)} blocks!")
        
        # Show some examples
        print(f"📝 Sample entries:")
        for i, block in enumerate(all_texture_blocks[:10]):
            print(f"  {i+1}. {block['block_name']} -> {block['texture_file']}")
        
        if len(all_texture_blocks) > 10:
            print(f"  ... and {len(all_texture_blocks) - 10} more blocks")
            
    except FileNotFoundError as e:
        print(f"❌ Error: {e}")
    except Exception as e:
        print(f"❌ Error processing files: {e}")

if __name__ == "__main__":
    main()
