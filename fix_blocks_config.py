import json

def fix_blocks_config():
    # Load the existing blocks configuration
    with open('blocks_config.json', 'r') as f:
        config = json.load(f)
    
    blocks = config['blocks']
    
    # Core blocks that should not be duplicated
    core_blocks = ['stone', 'dirt', 'grass', 'air']
    
    # Remove any core blocks from the generated list
    for core_block in core_blocks:
        if core_block in blocks:
            print(f"Removing duplicate core block: {core_block}")
            del blocks[core_block]
    
    # Save the fixed configuration
    config['blocks'] = blocks
    config['metadata']['total_blocks'] = len(blocks)
    
    with open('blocks_config.json', 'w') as f:
        json.dump(config, f, indent=2)
    
    print(f"Fixed blocks configuration - now has {len(blocks)} unique blocks")

if __name__ == "__main__":
    fix_blocks_config()
