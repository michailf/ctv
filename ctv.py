#!/usr/bin/python

import os, utils, etvapi, ui, time, subprocess, sys

DIR = os.environ['HOME'] + '/.cache/etvcc/list/'
utils.ensure_private_dir('.cache/etvcc/list/')

currtab = 0

def print_tabs(favs):
	cnt = 0
	s = ''
	for f in favs:
		if cnt == currtab:
			s += '\x1b[1;33m%d-%s\x1b[0m ' % (cnt+1, f['title'].encode('utf-8'))
		else:
			s += '\x1b[32m%d\x1b[0m-%s ' % (cnt+1, f['title'].encode('utf-8'))
		cnt += 1
	s += '\x1b[32m0\x1b[0m-quit\n'
	print s

def play_video(id, max_avail_bitrate):
	rc, url = etvapi.get_stream_url(id, max_avail_bitrate)
	print 'play_video:', url
	if rc == False:
		print 'press ENTER to continue'
		time.sleep(1)
		return 2

	if os.environ['HOME'] == '/home/pi':
		args = [
			'omxplayer',
			'-b',
			'--key-config',
			'/home/pi/bin/omxp_keys.txt',
			url
		]
	else:
		args = ['mplayer', '-cache-min', '64', url]

	p = subprocess.Popen(args)
	rc = p.wait()

	if rc == 0:
		return 0

	s = p.stdout.read()
	print 'stdout:', s
	s = p.stderr.read()
	print 'stderr:', s
	print 'press ENTER to continue'
	sys.stdin.readline()
	return 1

def get_max_bitrate(child):
	max = 0
	for f in child['files']:
		if f['format'] == 'mp4':
			if max < f['bitrate']:
				max = f['bitrate']
	return max

def loop(favs):
	global currtab
	f = favs[currtab]
	list = etvapi.get_bookmarks(f['id'])
	while True:
		ui.clear_screen()
		print_tabs(favs)
		ui.print_list(list)
		
		while True:
			ch = ui.getch()
			if ch in ['A', 'B', 'C', 'D', 'q', '0', '\r', '1', '2', '3', '4']:
				break

		if ch in [ '0', 'q' ] :
			break
		if ch > '0' and ch < '5':
			currtab = ord(ch) - 0x31
			f = favs[currtab]
			list = etvapi.get_bookmarks(f['id'])
			continue

		if ch == '\r':
			id = list[ui.curr]['id']

			if list[ui.curr]['type'] == 'MediaObject':
				ui.clear_screen()
				br = get_max_bitrate(list[ui.curr])
				if br > 0:
					play_video(id, br)
			else:
				max_num = list[ui.curr]['children_count']
				cnum = ui.read_int('%d.num' % id)
				
				while True:
					idx = max_num - cnum
					page = (idx / ui.PAGE_SIZE) + 1
					print 'id:', id, 'page:', page
					children = etvapi.get_children(id, page)
					childnum = idx - (page - 1) * ui.PAGE_SIZE
					cid = children[childnum]['id']
					print children[childnum]['short_name']
					time.sleep(2)
					ui.clear_screen()
					start_time = time.time()
					br = get_max_bitrate(children[childnum])
					if br > 0:
						rc = play_video(cid, br)
					else:
						rc = 1
					played_time = int(time.time() - start_time)
					cnum = cnum % max_num + 1
					if cnum > max_num - 1:
						cnum = max_num
					ui.write_int('%d.num' % list[ui.curr]['id'], cnum)
					if rc != 2 and (played_time < (children[childnum]['duration'] * 60 - 120)):
						break

		ui.move(list, ch)

def main():
	favs = etvapi.get_favorites()
	loop(favs)
	ui.clear_screen()

main()
