#!/usr/bin/env python3
"""
Rate-limited script to organize block assets into JSON format using OpenAI API.
Respects the 3 requests per minute rate limit.
"""

import csv
import json
import os
import time
from pathlib import Path
from dotenv import load_dotenv
import openai

# Load environment variables
load_dotenv()

def setup_openai():
    """Setup OpenAI client."""
    api_key = os.getenv('OPENAI_API_KEY')
    if not api_key:
        raise ValueError("OPENAI_API_KEY not found in environment variables")
    
    openai.api_key = api_key
    return openai

def read_block_assets():
    """Read the block assets CSV file."""
    csv_file = "block_assets.csv"
    if not Path(csv_file).exists():
        raise FileNotFoundError(f"CSV file '{csv_file}' not found. Run list_block_assets.py first.")
    
    assets = []
    with open(csv_file, 'r', encoding='utf-8') as file:
        reader = csv.DictReader(file)
        for row in reader:
            assets.append(row)
    
    return assets

def create_prompt(asset_batch):
    """Create a prompt for OpenAI to categorize the assets."""
    filenames = [asset['filename'] for asset in asset_batch]
    
    prompt = f"""
You are organizing Minecraft block texture files. I need you to categorize these {len(filenames)} texture filenames into a JSON structure.

Return the data as a flat object where each key is a unique block identifier and each value is a block object like this:

{{
    "acacia_door": {{
        "block_name": "Acacia Door",
        "textures": {{
            "top": "acacia_door_top.png",
            "bottom": "acacia_door_bottom.png"
        }}
    }},
    "acacia_log": {{
        "block_name": "Acacia Log",
        "textures": {{
            "sides": "acacia_log.png",
            "top": "acacia_log_top.png"
        }}
    }}
}}

Texture types to use: front, back, left, right, sides, top, bottom, all, other

Rules:
1. Group related textures by block type
2. Use descriptive block names (e.g., "Acacia Door" not "acacia_door")
3. Use snake_case for the object keys
4. Only include texture properties that actually apply to each file
5. If a texture doesn't clearly fit a face type, use "other"
6. Return ONLY valid JSON, no other text

Files to categorize:
{', '.join(filenames)}
"""
    
    return prompt

def process_batch_with_openai(client, asset_batch, batch_num, total_batches):
    """Process a batch of assets using OpenAI API."""
    print(f"🔄 Processing batch {batch_num}/{total_batches} ({len(asset_batch)} files)...")
    
    prompt = create_prompt(asset_batch)
    
    try:
        response = client.chat.completions.create(
            model="gpt-4o-mini",
            messages=[
                {"role": "system", "content": "You are a helpful assistant that organizes Minecraft texture files into structured JSON format."},
                {"role": "user", "content": prompt}
            ],
            temperature=0.1,
            max_tokens=2000
        )
        
        content = response.choices[0].message.content.strip()
        
        # Try to extract JSON from the response
        if content.startswith('```json'):
            content = content[7:]
        if content.endswith('```'):
            content = content[:-3]
        
        try:
            result = json.loads(content)
            print(f"✅ Batch {batch_num}/{total_batches} completed successfully")
            return result
        except json.JSONDecodeError as e:
            print(f"❌ Batch {batch_num}/{total_batches} - JSON parsing error: {e}")
            print(f"Response was: {content[:200]}...")
            return None
            
    except Exception as e:
        print(f"❌ Batch {batch_num}/{total_batches} - API error: {e}")
        return None

def main():
    """Main function to process all block assets with rate limiting."""
    print("🚀 Setting up OpenAI client...")
    client = setup_openai()
    
    print("📖 Reading block assets...")
    assets = read_block_assets()
    print(f"📁 Found {len(assets)} assets to process")
    
    # Process in batches of 25 (slightly larger since we're going slower)
    batch_size = 25
    all_blocks = {}
    successful_batches = 0
    failed_batches = 0
    
    total_batches = (len(assets) + batch_size - 1) // batch_size
    print(f"⏱️  Processing {total_batches} batches with rate limiting (3 requests/minute)")
    print(f"🕐 Estimated time: {total_batches * 20} seconds ({total_batches * 20 / 60:.1f} minutes)")
    
    for i in range(0, len(assets), batch_size):
        batch = assets[i:i + batch_size]
        batch_num = (i // batch_size) + 1
        
        result = process_batch_with_openai(client, batch, batch_num, total_batches)
        
        if result:
            # Handle different response formats
            if isinstance(result, dict):
                # Check if it's wrapped in a "blocks" key or similar
                if 'blocks' in result and isinstance(result['blocks'], list):
                    # Convert list to dict
                    for block in result['blocks']:
                        if isinstance(block, dict) and 'block_name' in block:
                            key = block['block_name'].lower().replace(' ', '_').replace('-', '_')
                            all_blocks[key] = block
                elif 'blocks' in result and isinstance(result['blocks'], dict):
                    all_blocks.update(result['blocks'])
                else:
                    # Direct format - merge the dictionary
                    all_blocks.update(result)
            elif isinstance(result, list):
                # Handle list format
                for block in result:
                    if isinstance(block, dict) and 'block_name' in block:
                        key = block['block_name'].lower().replace(' ', '_').replace('-', '_')
                        all_blocks[key] = block
            
            successful_batches += 1
        else:
            failed_batches += 1
        
        # Rate limiting - wait 20 seconds between requests (3 per minute = 1 every 20 seconds)
        if i + batch_size < len(assets):
            print(f"⏳ Waiting 20 seconds for rate limit (batch {batch_num}/{total_batches} done)")
            time.sleep(20)
    
    # Save results to JSON file
    output_file = "organized_block_assets_final.json"
    try:
        # Clean and organize the final data
        final_data = {}
        for key, block in all_blocks.items():
            # Ensure consistent structure
            if isinstance(block, dict) and 'block_name' in block:
                clean_key = key.replace(' ', '_').lower()
                final_data[clean_key] = block
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(final_data, f, indent=2, ensure_ascii=False)
        
        print(f"\n🎉 Successfully created '{output_file}' with {len(final_data)} organized blocks!")
        print(f"📊 Results: {successful_batches} successful batches, {failed_batches} failed batches")
        
        if final_data:
            print(f"📁 Sample entries:")
            for i, (key, block) in enumerate(list(final_data.items())[:3]):
                print(f"  {i+1}. {block.get('block_name', key)}")
                textures = block.get('textures', {})
                for face, texture in textures.items():
                    if isinstance(texture, str):
                        print(f"     {face}: {texture}")
                    elif isinstance(texture, list):
                        print(f"     {face}: {', '.join(texture)}")
        
    except Exception as e:
        print(f"❌ Error saving JSON file: {e}")

if __name__ == "__main__":
    main() 