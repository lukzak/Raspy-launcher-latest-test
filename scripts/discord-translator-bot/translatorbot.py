#### Need to install google trans with pip - pip install googletrans==3.1.0a0  It MUST be this version. Not the newer 4.x whatever version
import discord
from discord.ext import commands
from googletrans import Translator
import os
import re
TOKEN = "XXXXX123 insert bot token here in the quotes"

intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix="!", intents=intents)

translator = Translator()

# Replace with your actual channel IDs
SOURCE_CHANNEL_ID = 0000000000000000000
TARGET_CHANNEL_ID = 0000000000000000000

@bot.event
async def on_ready():
    print(f"Logged in as {bot.user}!")

@bot.event
async def on_message(message):
    if message.author == bot.user:
        return

    if message.channel.id == SOURCE_CHANNEL_ID:
        content = message.content

        # Step 1: Split content into parts: plain text and markdown links
        parts = []
        last_end = 0

        for match in re.finditer(r'\[([^\]]+)\]\(([^)]+)\)', content):
            start, end = match.span()
            link_text, link_url = match.groups()

            # Add the text before the link
            if start > last_end:
                parts.append(("text", content[last_end:start]))

            # Add the link (type, text, url)
            parts.append(("link", link_text, link_url))

            last_end = end

        # Add remaining text after last match
        if last_end < len(content):
            parts.append(("text", content[last_end:]))

        # Step 2: Translate plain text and link text
        output = ""
        for part in parts:
            if part[0] == "text":
                translated = translator.translate(part[1], dest="de").text
                output += translated
            elif part[0] == "link":
                translated_text = translator.translate(part[1], dest="de").text
                output += f"[{translated_text}]({part[2]})"

        # Step 3: Send to target channel
        target_channel = bot.get_channel(TARGET_CHANNEL_ID)
        if target_channel:
            await target_channel.send(output)

    await bot.process_commands(message)

bot.run(TOKEN)
