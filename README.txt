######### Discord Bot News/Translator Setup

This assumes you have a working webserver that can serve files publicly over the internet. It also assumes you know how to add a Discord bot to your server, set its permissions, and obtain a bot token.

The news function works by displaying an HTML document in the launcher. You can point it to whatever website you want, but it is designed to work with the included Discord news bot.

Setup steps:

1. Set up the Discord news bot so it generates the HTML document your launcher will read.
2. When ready to build the launcher, put the URL of this HTML document into the manual config section at the top of the main.cpp file.

Tip: It is recommended to create a private Discord server to follow news channels from your main server. This lets you post test messages without them appearing to all players.

Important: Restrict your announcement channel so only trusted people can post. Any messages from others will be pushed to the launcher and may require manual removal using the !clear_last 1 command after deleting them in Discord.

When inviting the bot to your Discord server:

- Give it permissions to read and write messages.
- Consider creating a separate channel named bot-commands to manage messages displayed by the news bot.
- Match your channel names exactly in the bot’s Python script.
- The channel name will appear as part of the headline and timestamp shown to players, so choose names thoughtfully.

If you want multi-language support:

- Replace the German channel names in the Python script with your own language names and codes.
- Update any references to "German" or "de" accordingly. The timestamp timezone code should be changed if you are not in the CET timezone.
- Insert your bot token in client.run('INSERT BOT TOKEN HERE') at the bottom of the script.
- The bot writes and updates the news HTML file with timestamps and channel names.
- Adjust the output file location if needed — make sure the bot has permission to write there.
- Open the news file URL in your browser to verify the HTML output (encoding errors may appear but it should display correctly in the launcher).

Translator Bot:

A separate Discord bot is included to translate messages using Google Translate and output them into another channel.

- This works best on a private Discord server mirroring the official one.
- Both input and output channels must be on the same server.
- Edit the script to set your target language and channel IDs (right-click channels to get IDs).
- Run the translator bot alongside the news bot.
- The translator bot processes official announcements and posts translated versions for the news bot to parse.
- Don’t forget to add your translator bot’s token at the top of its Python script.
- Make sure to install googletrans. It MUST be version 3.1.0a0 -  pip install googletrans==3.1.0a0

Testing and maintenance:

During testing, you might post many messages. A private server helps keep your main server clean.

Use the !clear_all and !clear_last X commands in the bot-commands channel to clear messages from the news file.

Note: The bot cannot detect edits or deletions in Discord automatically. You must manually clear and repost edited messages.

After setup, consider running the bots in the background with your webserver’s crontab to start on reboot.

To populate the launcher without posting a needless news update, copy-paste the last news from your offical channel and paste it in your private channel. 
You can then modify the timestamp in the latest_news file to backdate it to the original message post date.

------------------------------------------------------------------

######### Launcher Configuration and Compilation

1. Build Qt 6.9.0 from source with static flags (assumes MinGW).
2. Build minizip-ng 4.0.9 with static flags using MinGW.
3. Edit CMakeLists.txt to set paths for minizip and zlib libraries/includes (avoid spaces in paths).
4. Customize the button asset with your project name. Use the included blank button. The font download URL is in the assets folder with the buttons. Make sure to name your button "button_website.bmp"
5. Update main.cpp with URLs for your news files and other assets:
   - If using multi-language support, set the base URL without the language code suffix (e.g., "https://yourwebserver.com/latest_news_").
   - If English only, use the full URL (e.g., "https://yourwebserver.com/latest_news_EN").
6. Use direct/raw links for all URLs.
7. Test your update URL with a dummy executable to ensure it downloads and replaces the launcher correctly.
8. Build in release mode (do NOT use “minimum size” in Qt Creator as it produces a larger binary with this project).

------------------------------------------------------------------

######### Addon Sync Repo

After building the launcher, you may want to create an addon repository for players.

1. Zip your addons carefully:
   - Each addon should be zipped in its own folder (e.g., Decursive.zip/Decursive/Binding.xml).
   - If an addon uses multiple folders (e.g., DPSMate), zip them together under one archive.
   - Don't use spaces in the zip file names - the names are used to build URLs

2. Upload your zipped addons to your webserver in one folder.
3. Test download links in your browser (they should be direct links that trigger immediate downloads).
4. Run the included repo.sh or repo.bat script in the folder with the .zip files.
5. Provide the output repo name and the direct URL to the folder containing the zipped addons. The script will build the download URLs by appending the zip file names to the folder URL.
6. The repo file will look like:

!addon_repo
Repo name: <my repo>
<addons>
[Decursive]::https://myserver.com/downloads/addons/Decursive.zip
[DPSMate]::https://myserver.com/downloads/addons/DPSMate.zip
...
</addons>

7. You can edit addon names inside the []s to add spaces or notes (e.g., [Decursive - RaspyWoW version] or [DPS Mate] instead of [DPSMate]).
8. The repo URLs can point to different servers or even GitHub release links (provided that the addon creator zips their addon in the way that the unzip function expects).
9. Host the repo file on your webserver or GitHub (using raw URLs).
10. Players add your repo URL in the Addon Sync window to see and download addons.

Note: The script does not support addons zipped with -master in folder names; these must be handled manually.
