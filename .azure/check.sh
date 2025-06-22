#!/bin/bash

# Script to verify the integrity of Python wheel files after downloading.
# Prevents the high cost of publishing a bad wheel.

# Ensure 'wheel' is installed
pip show wheel >/dev/null 2>&1 || pip install wheel

for whl in *.whl; do
    echo "======================================"
    echo "Checking wheel: $whl"
    echo "--------------------------------------"
    
    # 1. List contents
    unzip -l "$whl"
    
    # 2. Unpack wheel and capture output to get directory name
    unpack_output=$(python3 -m wheel unpack "$whl")
    echo "$unpack_output"
    unpack_dir=$(echo "$unpack_output" | grep "Unpacking to:" | sed 's/Unpacking to: //' | sed 's/\.\.\..*//')
    unpack_dir=$(echo "$unpack_dir" | xargs)  # trim whitespace

    # 3. Check for METADATA and WHEEL files
    dist_info_dir=$(find "$unpack_dir" -type d -name "*.dist-info" | head -n 1)
    if [ -n "$dist_info_dir" ]; then
        echo "Found dist-info directory: $dist_info_dir"
        if [ -f "$dist_info_dir/METADATA" ]; then
            echo "METADATA file present."
        else
            echo "WARNING: METADATA file missing!"
        fi
        if [ -f "$dist_info_dir/WHEEL" ]; then
            echo "WHEEL file present."
        else
            echo "WARNING: WHEEL file missing!"
        fi
    else
        echo "WARNING: dist-info directory not found!"
    fi

    # 4. Auditwheel check (if available)
    if command -v auditwheel >/dev/null 2>&1; then
        auditwheel show "$whl"
    fi

    # Clean up unpacked directory
    rm -rf "$unpack_dir"
    echo "======================================"
    echo
done
