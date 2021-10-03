#!/usr/bin/env python3
import anitopy
import sys
import os
import pyperclip
import argparse

# Parse input arguments
desc = 'Searches for video files with a specific anime title, and copies a ffmpeg command to copy a section of it'
parser = argparse.ArgumentParser(description=desc)
parser.add_argument('--ep', type=int)
parser.add_argument('--root', type=str)
parser.add_argument('search')
parser.add_argument('minutes', type=int, default=0)
parser.add_argument('seconds', type=int, default=0)

args = parser.parse_args()
root = args.root
key = args.search
ep = args.ep



# Search for and parse anime filenames
found_anime = []

def parse(name):
	try:
		return anitopy.parse(name)
	except Exception as e:
		return None

for root, dir, files in os.walk(root):
	for file in files:
		parsed = parse(file)
		if parsed and 'anime_title' in parsed:
			parsed['dir'] = root
			found_anime.append(parsed)
	

# Select potential matches	
matches = []

for anime in found_anime:
	if key.lower() in anime['anime_title'].lower():
		if 'episode_number' in anime:
			number = anime['episode_number']
			if number.isnumeric() and int(anime['episode_number']) == ep:
				matches.append(anime)
		
# Ask user for the file they want to use
print('Found following files:')
for id, match in enumerate(matches):
	#print('%d:\t%s/%s' % (id, match['dir'], match['file_name'])) # With parent dir in filepath
	print('%d:\t%s' % (id, match['file_name']))
	
id = 0
if (len(matches) > 1):
	print('\nPick a file:')
	id = int(input())

#print(matches[id])


# Copy ffmpeg command to clipboard
match = matches[id]
filename = '%s/%s' % (match['dir'], match['file_name'])
cmd = 'ffmpeg -i "%s" -c copy -an -sn -t 00:00:10.0 -ss 00:%.2d:%.2d "%s-ep%s-01.mkv"' % (filename, args.minutes, args.seconds, match['anime_title'], match['episode_number'])
print(cmd)
pyperclip.copy(cmd)
print('FFMPEG command copied to clipboard')
