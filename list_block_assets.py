#!/usr/bin/env python3
"""
Script to list all filenames in the assets/block directory and save them to a CSV file.
"""

import os
import csv
from pathlib import Path

def list_block_assets():
    """
    Traverse the assets/block directory and collect all filenames.
    Save the results to a CSV file.
    """
    # Define paths
    block_assets_dir = Path("assets/block")
    output_csv = "block_assets.csv"
    
    # Check if the directory exists
    if not block_assets_dir.exists():
        print(f"Error: Directory '{block_assets_dir}' does not exist!")
        return
    
    # Collect all filenames
    filenames = []
    
    # Walk through the directory and subdirectories
    for root, dirs, files in os.walk(block_assets_dir):
        for file in files:
            # Get the relative path from the block directory
            relative_path = os.path.relpath(os.path.join(root, file), block_assets_dir)
            filenames.append({
                'filename': file,
                'relative_path': relative_path,
                'full_path': os.path.join(root, file)
            })
    
    # Sort filenames alphabetically
    filenames.sort(key=lambda x: x['filename'])
    
    # Write to CSV
    try:
        with open(output_csv, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['filename', 'relative_path', 'full_path']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            # Write header
            writer.writeheader()
            
            # Write data
            for file_info in filenames:
                writer.writerow(file_info)
        
        print(f"Successfully created '{output_csv}' with {len(filenames)} files.")
        print(f"First few files:")
        for i, file_info in enumerate(filenames[:5]):
            print(f"  {i+1}. {file_info['filename']}")
        if len(filenames) > 5:
            print(f"  ... and {len(filenames) - 5} more files")
            
    except Exception as e:
        print(f"Error writing CSV file: {e}")

if __name__ == "__main__":
    list_block_assets() 