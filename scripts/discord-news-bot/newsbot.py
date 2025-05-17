## This bot is designed to read text from a discord channel and add it to a file called "latest_news_EN"
import discord
import datetime
import re
import markdown

# Define your bot client
intents = discord.Intents.all()
client = discord.Client(intents=intents)

# Channels to listen for bot commands and logging
bot_commands_channel = 'bot-commands'
channels_to_log = ['update', 'announcement', 'ankündigung', 'neuigkeit']  # Include German channel here

# File paths for different languages
log_files = {
    'default': '/var/www/html/latest_news_EN',
    'de': '/var/www/html/latest_news_DE'
}

# Function to determine the log file based on channel name. Replace 'ankündigung', 'neuigkeit' with your foreign language channel names.
def get_log_file(channel_name):
    if channel_name in ('ankündigung', 'neuigkeit'):
        return log_files['de']
    return log_files['default']

# Function to write a new entry to the log
def build_html_entry(timestamp, channel_name, content):
    return f"""<div class="news-entry">
    <p><strong><span style="font-size: 18px;">[{timestamp} CET] {channel_name}</span></strong>
    <span style="font-size: 13px;">{content}</span></p>
</div>
"""

@client.event
async def on_ready():
    print(f'Logged in as {client.user}')

@client.event
async def on_message(message):
    channel_name = message.channel.name

    # Handle bot commands
    if channel_name == bot_commands_channel:
        if message.content.startswith('!clear_all'):
            for path in log_files.values():
                with open(path, 'w') as contents:
                    contents.write("<html><body>\n</body></html>")
            await message.channel.send("All logs have been cleared.")

        elif message.content.startswith('!clear_last'):
            try:
                num_entries = int(message.content.split()[1])

                for path in log_files.values():
                    try:
                        with open(path, 'r') as contents:
                            lines = contents.readlines()
                    except FileNotFoundError:
                        continue

                    timestamped_entries = []
                    entry_start = False
                    current_entry = []

                    for line in lines:
                        if '<div class="news-entry">' in line:
                            entry_start = True
                            current_entry = [line]
                        elif '</div>' in line and entry_start:
                            current_entry.append(line)
                            match = re.search(r'\[(.*?) CET\]', ''.join(current_entry))
                            if match:
                                timestamp = match.group(1)
                                timestamp_obj = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M")
                                timestamped_entries.append((timestamp_obj, ''.join(current_entry)))
                            entry_start = False
                        elif entry_start:
                            current_entry.append(line)

                    if len(timestamped_entries) < num_entries:
                        continue

                    timestamped_entries.sort(reverse=True, key=lambda x: x[0])
                    updated_entries = [entry[1] for entry in timestamped_entries[num_entries:]]

                    updated_html = "<html><body>\n" + ''.join(updated_entries) + "\n</body></html>"
                    with open(path, 'w') as contents:
                        contents.write(updated_html)

                await message.channel.send(f"Removed the last {num_entries} entries from all logs.")
            except (IndexError, ValueError):
                await message.channel.send("Please specify a valid number after `!clear_last`.")

    elif channel_name in channels_to_log:
        log_file = get_log_file(channel_name)
        timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
        channel_display = channel_name.capitalize()

        try:
            with open(log_file, 'r') as contents:
                existing_html = contents.read()
        except FileNotFoundError:
            existing_html = "<html><body>\n</body></html>"

        if existing_html == "<html><body>\n</body></html>":
            existing_html = ""

        message_content_html = markdown.markdown(message.content)
        new_entry = build_html_entry(timestamp, channel_display, message_content_html)

        with open(log_file, 'w') as contents:
            if "<html><body>" not in existing_html:
                contents.write("<html><body>\n")
            contents.write(new_entry)
            contents.write(existing_html)
            if "</body></html>" not in existing_html:
                contents.write("\n</body></html>")
client.run('INSERT BOT TOKEN HERE IN THE QUOTES')
