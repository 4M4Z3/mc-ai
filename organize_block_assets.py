#!/usr/bin/env python3
"""
Script to organize block assets into JSON format using OpenAI API for intelligent categorization.
Processes assets in batches of 20 and categorizes by face types.
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

Each block should be organized like this:
{{
    "block_name": "Readable Block Name",
    "textures": {{
        "front": "filename.png" (if applicable),
        "back": "filename.png" (if applicable),
        "left": "filename.png" (if applicable),
        "right": "filename.png" (if applicable),
        "sides": "filename.png" (if applicable - for textures used on multiple sides),
        "top": "filename.png" (if applicable),
        "bottom": "filename.png" (if applicable),
        "all": "filename.png" (if same texture used for all faces),
        "other": "filename.png" (for special variants like powered, lit, etc.)
    }}
}}

Rules:
1. Group related textures by block type (e.g., all acacia door textures go together)
2. Use descriptive block names (e.g., "Acacia Door" not "acacia_door")
3. Only include texture properties that actually apply to each file
4. If a texture doesn't clearly fit a face type, use "other"
5. If multiple files exist for the same block, group them appropriately
6. Return ONLY valid JSON, no other text

Files to categorize:
{', '.join(filenames)}

Return the JSON structure:
"""
    
    return prompt

def process_batch_with_openai(client, asset_batch):
    """Process a batch of assets using OpenAI API."""
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
            return json.loads(content)
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON response: {e}")
            print(f"Response content: {content}")
            return None
            
    except Exception as e:
        print(f"Error calling OpenAI API: {e}")
        return None

def main():
    """Main function to process all block assets."""
    print("Setting up OpenAI client...")
    client = setup_openai()
    
    print("Reading block assets...")
    assets = read_block_assets()
    print(f"Found {len(assets)} assets to process")
    
    # Process in batches of 20
    batch_size = 20
    all_blocks = {}
    
    for i in range(0, len(assets), batch_size):
        batch = assets[i:i + batch_size]
        batch_num = (i // batch_size) + 1
        total_batches = (len(assets) + batch_size - 1) // batch_size
        
        print(f"Processing batch {batch_num}/{total_batches} ({len(batch)} files)...")
        
        result = process_batch_with_openai(client, batch)
        
        if result:
            # Merge results
            if isinstance(result, dict):
                all_blocks.update(result)
            elif isinstance(result, list):
                for block in result:
                    if isinstance(block, dict) and 'block_name' in block:
                        block_key = block['block_name'].lower().replace(' ', '_')
                        all_blocks[block_key] = block
        
        # Rate limiting - wait a bit between requests
        if i + batch_size < len(assets):
            print("Waiting 1 second to respect rate limits...")
            time.sleep(1)
    
    # Save results to JSON file
    output_file = "organized_block_assets.json"
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(all_blocks, f, indent=2, ensure_ascii=False)
        
        print(f"\nSuccessfully created '{output_file}' with {len(all_blocks)} organized blocks!")
        print(f"Sample entries:")
        for i, (key, block) in enumerate(list(all_blocks.items())[:3]):
            print(f"  {i+1}. {block.get('block_name', key)}")
            textures = block.get('textures', {})
            for face, texture in textures.items():
                print(f"     {face}: {texture}")
        
    except Exception as e:
        print(f"Error saving JSON file: {e}")

if __name__ == "__main__":
    main() 