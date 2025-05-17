#!/bin/bash

# Get user details
echo "Enter the name of your addon repo: "
read repo_name
echo "Enter the location of your addon folder with .zip files. eg. https://example.com/addons "
read domain
echo "Enter a file output name. This is the remote file that Addon Sync will read to get the download URLs. It can have a .txt extension or no extension: "
read output_file

# Clear the output file if it already exists
> "$output_file"

# Add the first lines to the output file
echo "!addon_repo" > "$output_file"
echo "Repo name: <$repo_name>" >> "$output_file"
echo "<addons>" >> "$output_file"

# Loop through all .zip files in the current directory
for zip_file in $(ls *.zip | sort -t'_' -k1,1 -f); do
    # Get the addon name (without extension)
    addon_name="${zip_file%.zip}"
    
    # Construct the URL
    url="$domain/$addon_name.zip"
    
    # Write the line to the output file
    echo "[$addon_name]::$url" >> "$output_file"
done

# Add the closing tag
echo "</addons>" >> "$output_file"

echo "Addons list has been saved to $output_file. Put this file on your webserver. The location of that file is what users will type into their addon manager to see your repo."
