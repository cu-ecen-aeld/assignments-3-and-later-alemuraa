#!/bin/bash

# Check: exactly two arguments
if [ $# -ne 2 ]; then
  echo "Usage: $0 <file_path> <text_string>"
  exit 1
fi

writefile=$1
writestr=$2

# Create directory if needed
mkdir -p "$(dirname "$writefile")"

# Write to the file (overwrite mode)
echo "$writestr" > "$writefile"

# Check if successful
if [ $? -ne 0 ]; then
  echo "Error: Could not write to file $writefile"
  exit 1
fi

