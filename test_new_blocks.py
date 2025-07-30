import json

def get_sample_blocks():
    """Get a few sample blocks for testing"""
    with open('blocks_config.json', 'r') as f:
        config = json.load(f)
    
    blocks = config['blocks']
    
    # Get first 5 blocks for testing
    sample_blocks = []
    count = 0
    for block_key, block_data in blocks.items():
        if count >= 5:
            break
        sample_blocks.append({
            'key': block_key,
            'name': block_data['blockName'],
            'id': block_data['blockType'],
            'texture': block_data['textures']['all']
        })
        count += 1
    
    print("Sample blocks for testing:")
    for block in sample_blocks:
        print(f"  {block['key']} (ID: {block['id']}) - {block['name']}")
        print(f"    Texture: {block['texture']}")
        print()
    
    return sample_blocks

if __name__ == "__main__":
    get_sample_blocks()
