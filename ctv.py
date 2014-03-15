#!/usr/bin/python

import os, utils, etvapi, ui, time, subprocess

DIR = os.environ['HOME'] + '/.cache/etvcc/list/'
utils.ensure_private_dir('.cache/etvcc/list/')

currtab = 0

def print_tabs(favs):
	cnt = 0
	for f in favs:
		m1 = ' '
		m2 = ' '
		if cnt == currtab:
			m1 = '\x1b[1;33m*'
			m2 = '\x1b[0m'
		print m1, cnt+1, f['title'].encode('utf-8'), '   ', m2,
		cnt += 1
	print

def play_video(id):
	rc, url = etvapi.get_stream_url(id)
	if rc == False:
		time.sleep(1)
		return 2

	if os.environ['HOME'] == '/home/pi':
		args = [
			'omxplayer',
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
			if ch in ['A', 'B', 'C', 'D', 'q', '0', '\r', '1', '2', '3']:
				break

		if ch in [ '0', 'q' ] :
			break
		if ch > '0' and ch < '4':
			currtab = ord(ch) - 0x31
			print currtab
			f = favs[currtab]
			list = etvapi.get_bookmarks(f['id'])
			continue

		if ch == '\r':
			id = list[ui.curr]['id']

			if list[ui.curr]['type'] == 'MediaObject':
				play_video(id)
			else:
				cnum = ui.read_int('%d.num' % id)
				max_num = list[ui.curr]['children_count']
				page = (cnum / 20) + 1
				children = etvapi.get_children(id, page)
				id = children[cnum - (page-1)*20 - 1]['id']
				play_video(id)
				cnum = (cnum - 1) % max_num
				if cnum == 0:
					cnum = 1
				ui.write_int('%d.num' % list[ui.curr]['id'], cnum)

		ui.move(list, ch)

def main():
	favs = etvapi.get_favorites()
	loop(favs)

main()
