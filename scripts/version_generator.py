#!/usr/bin/env python3
"""
Standalone version generator script for SRDriver
Run this manually when you want to update the version information.
"""

import os
import datetime
import subprocess
import sys

def get_git_info():
    """Extract git information for versioning"""
    try:
        # Get short git hash (7 characters is the standard)
        git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], 
                                         cwd=os.getcwd(), stderr=subprocess.DEVNULL).decode().strip()
    except:
        git_hash = "unknown"
    
    try:
        # Get latest tag
        git_tag = subprocess.check_output(['git', 'describe', '--tags', '--abbrev=0'], 
                                        cwd=os.getcwd(), stderr=subprocess.DEVNULL).decode().strip()
    except:
        git_tag = "dev"
    
    return git_tag, git_hash

def generate_version_header():
    """Generate version.h file with current version information"""
    git_tag, git_hash = get_git_info()
    
    # Create timestamp
    now = datetime.datetime.now()
    build_date = now.strftime('%Y-%m-%d')
    build_time = now.strftime('%H:%M:%S')
    build_timestamp = now.strftime('%Y%m%d-%H%M%S')
    
    # Build version string (simplified without platform info)
    version = f"{git_tag}-{git_hash}-{build_timestamp}"
    
    print(f"Generated firmware version: {version}")
    
    # Create version.h file content
    version_content = f"""#pragma once

// Auto-generated version information - DO NOT EDIT
// Generated on {build_date} at {build_time}
#define FIRMWARE_VERSION "{version}"
#define VERSION_TAG "{git_tag}"
#define VERSION_HASH "{git_hash}"
#define BUILD_DATE "{build_date}"
#define BUILD_TIME "{build_time}"
#define BUILD_TIMESTAMP "{build_timestamp}"
"""
    
    # Write to version.h file
    version_file = os.path.join('src', 'version.h')
    
    # Only write if content has changed
    try:
        with open(version_file, 'r') as f:
            existing_content = f.read()
        if existing_content == version_content:
            print("Version unchanged, skipping file update")
            return
    except FileNotFoundError:
        pass
    
    # Write new version file
    with open(version_file, 'w') as f:
        f.write(version_content)
    
    print(f"Updated {version_file}")

if __name__ == "__main__":
    generate_version_header()
