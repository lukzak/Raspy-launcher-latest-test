@echo off
setlocal enabledelayedexpansion

:: Ask the user for the output file and repo name
set /p repo_name="Enter the name of your addon repo: "
set /p domain="Enter the location of your addon folder with .zip files. eg. https://example.com/addons: "
set /p output_file="Enter a file output name. This is the remote file that Addon Sync will read to get the download URLs. It can have a .txt extension or no extension: "

:: Create a temp file to avoid trailing newline
set "tmp_file=%output_file%.tmp"

:: Start the temp file
(
    setlocal disabledelayedexpansion
    echo !addon_repo
    endlocal
    echo Repo name: ^<%repo_name%^>
    echo ^<addons^>
) > "%tmp_file%"

:: Loop through all .zip files in the current directory
for /f "delims=" %%f in ('dir /b /o:n *.zip') do (
    set "addon_name=%%~nf"
    set "url=%domain%/%%~nxf"
    echo [!addon_name!]::!url!>> "%tmp_file%"
)

:: Now write the final line exactly, with no newline or trailing space
<nul set /p="</addons>" >> "%tmp_file%"

:: Rename temp file to final output
move /y "%tmp_file%" "%output_file%" > nul

echo Addons list has been saved to "%output_file%. Put this file on your webserver. The location of that file is what users will type into their addon manager to see your repo."

endlocal