#!/bin/bash

# File name and target size
file="$1"
target_size=$((500 * 1024 * 1024))  # 500MB in bytes

echo "Expanding $file to $target_size bytes"

# Get the current size of the file
current_size=$(stat -c%s "$file")

# Loop until the file reaches the target size
while [ "$current_size" -lt "$target_size" ]; do
	echo "Current size: $current_size"
    cat "$file" >> "$file.tmp"  # Append the file to a temporary file
    current_size=$(stat -c%s "$file.tmp")  # Update the current size
done

mv "$file.tmp" expanded_"$file"      # Replace the original file with the new one

echo "File has been expanded to $current_size bytes."

