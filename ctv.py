#!/usr/bin/python

import os, utils, etvapi, ui

DIR = os.environ['HOME'] + '/.cache/etvcc/list/'
utils.ensure_private_dir('.cache/etvcc/list/')

currtab = 0

def mkdir(parent, f):
	d = DIR + parent + '/' + str(f['id'])
	if not os.path.exists(d):
		os.mkdir(d)

def print_tabs(favs):
	cnt = 0
	for f in favs:
		m = ' '
		if cnt == currtab:
			m = '*'
		print m, f['title'].encode('utf-8'), '   ',
		cnt += 1
	print
	
def loop(favs):
	print_tabs(favs)
	f = favs[currtab]
	list = etvapi.get_bookmarks(f['id'])
	for b in list:
		mkdir(str(f['id']), b)

	list = os.listdir(DIR + str(f['id']))
	ui.loop(DIR + str(f['id']) + '/', list)

def main():
	favs = etvapi.get_favorites()
	loop(favs)

main()
