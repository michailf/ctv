#!/usr/bin/python

# Console player for etvnet.com for using mostly on Raspberry PI.
# To activate run ctv-activate.py first.

import os, utils, etvapi, ui, time, subprocess, sys

currtab = 0
green = '\x1b[32m'
bold_yellow = '\x1b[1;33m'
nocolor = '\x1b[0m'

def print_tabs(favs):
	cnt = 0
	s = ''
	for f in favs:
		title = '%d-%s' % (cnt + 1, f['title'].encode('utf-8'))
		if cnt == currtab:
			s += bold_yellow + title + nocolor + ' '
		else:
			s += green + title + nocolor + ' '
		cnt += 1
	s += green + '0' + nocolor + '-quit\n'
	print s

def run_player(id, max_avail_bitrate):
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

def play_single_video(child):
	ui.clear_screen()
	br = get_max_bitrate(child)
	if br > 0:
		rc = run_player(cid, br)
	else:
		rc = 1
	return rc

def play_all_container(list):
	max_num = list[ui.curr]['children_count']
	cnum = ui.read_int('%d.num' % id)

	while True:
		idx = max_num - cnum
		page = (idx / ui.PAGE_SIZE) + 1
		children = etvapi.get_children(id, page)
		childnum = idx - (page - 1) * ui.PAGE_SIZE
		cid = children[childnum]['id']
		print children[childnum]['short_name']
		time.sleep(2)
		start_time = time.time()
		rc = play_single_video(children[childnum])
		played_time = int(time.time() - start_time)
		cnum = cnum % max_num + 1
		if cnum > max_num - 1:
			cnum = max_num
		ui.write_int('%d.num' % list[ui.curr]['id'], cnum)
		if rc != 2 and (played_time < (children[childnum]['duration'] * 60 - 120)):
			break
	return rc

def switch_favorites_tab(favs, ch):
	currtab = ord(ch) - 0x31
	if (currtab >= len(favs)):
		currtab = 0
	f = favs[currtab]
	list = etvapi.get_bookmarks(f['id'])
	return currtab, f, list

def loop(favs):
	global currtab
	f = favs[currtab]
	list = etvapi.get_bookmarks(f['id'])

	while True:
		ui.clear_screen()
		print_tabs(favs)
		ui.print_list(list)
		ch = ui.getch()

		if ch in [ '0', 'q' ] :
			break

		if ch > '0' and ch < '5':
			currtab, f, list = switch_favorites_tab(favs, ch)
			continue

		if ch != '\r':
			ui.move(list, ch)
			continue

		if list[ui.curr]['type'] == 'MediaObject':
			play_single_video(list[ui.curr])
		else:
			play_all_container(list)


def main():
	favs = etvapi.get_favorites()
	loop(favs)
	ui.clear_screen()

main()
