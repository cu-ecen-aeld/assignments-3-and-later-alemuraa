#!/bin/bash

# Check if exactly 2 arguments are provided
if [ $# -ne 2 ]; then
  echo "Usage: $0 <directory_path> <search_string>"
  exit 1
fi

filesdir=$1     # First argument: directory path
searchstr=$2    # Second argument: string to search

# Check if the provided path is a valid directory
if [ ! -d "$filesdir" ]; then
  echo "Error: '$filesdir' is not a valid directory."
  exit 1
fi

# Count the number of files in the directory and subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of lines in those files that contain the search string
match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Print the result in the required format
echo "The number of files are $file_count and the number of matching lines are $match_count"

