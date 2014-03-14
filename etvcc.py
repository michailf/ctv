#!/usr/bin/python

import json, sys, os, subprocess, urllib, shutil, hashlib, urllib2, datetime, bbctrl, time

# ==== constants ===================================
client_id = "a332b9d61df7254dffdc81a260373f25592c94c9"
client_secret = '744a52aff20ec13f53bcfd705fc4b79195265497'
oauth_url = 'https://accounts.etvnet.com/auth/oauth/token'
api_root = 'https://secure.etvnet.com/api/v3.0/'
scope = 'com.etvnet.media.browse com.etvnet.media.watch com.etvnet.media.bookmarks com.etvnet.media.history \
com.etvnet.media.live com.etvnet.media.fivestar com.etvnet.media.comments com.etvnet.persons com.etvnet.notifications'
# ==================================================

def read_param(name, default = None):
	fname = os.environ['HOME'] + '/.config/etvcc/' + name + '.txt'

	if not os.path.exists(fname) and default is not None:
		return default

	f = open(fname)
	s = f.readline().strip()
	f.close()
	return s

# ==== params =====================================
access_token = read_param('access_token')
bitrate = int(read_param('bitrate', 400))
debug = int(read_param('debug', 0))
# ==================================================


def write_param(name, value):
	f = open(os.environ['HOME'] + '/.config/etvcc/' + name + '.txt', 'wt')
	f.write(value + '\n')
	f.close()


def append_cached(name, value):
	f = open(os.environ['HOME'] + '/.cache/etvcc/' + name + '.txt', 'at')
	f.write(str(value) + '\n')
	f.close()


def read_cached_list(name):
	fname = os.environ['HOME'] + '/.cache/etvcc/' + name + '.txt'

	if not os.path.exists(fname):
		return []

	f = open(fname)
	list = f.readlines()
	f.close()
	return list


def add_history(id, pos):
	f = open(os.environ['HOME'] + '/.cache/etvcc/history.txt', 'at')
	f.write('%d,%d\n' % (id, pos))
	f.close()


def read_history(): 
	fname = os.environ['HOME'] + '/.cache/etvcc/history.txt'

	if not os.path.exists(fname):
		return {}

	f = open(fname, 'rt')
	lines = f.readlines()
	history = {}
	for l in lines:
		c = l.split(',')
		history[int(c[0])] = [int(c[1])]
	return history


def read_json(fname):
	f = open(fname)
	text = f.read()
	f.close()
	a = json.loads(text)
	return a


def refresh_token():
	data = {
		'client_id': client_id,
		'client_secret': client_secret,
		'grant_type': 'refresh_token',
		'refresh_token': read_param('refresh_token'),
		'scope': scope
		}

	udata = urllib.urlencode(data)
	req = urllib2.Request(oauth_url)
	resp = urllib2.urlopen(req, udata).read()
	if debug:
		print 'refresh_token resp:', resp
	a = json.loads(resp)
	write_param('access_token', a['access_token'])
	write_param('refresh_token', a['refresh_token'])


def get_cached(url, name):
	global access_token
	md5 = hashlib.md5(url).hexdigest()
	fname = os.environ['HOME'] + '/.cache/etvcc/' + name + '-' + md5 + '.json'
	if not os.path.exists(fname):
		tmpname = ''
		if debug:
			print 'loading %s: %s' % (name, url)
		try:
			(tmpname, headers) = urllib.urlretrieve(url)
		except:
			refresh_token()
			access_token = read_param('access_token')
			(tmpname, headers) = urllib.urlretrieve(url)
		shutil.move(tmpname, fname)
	a = read_json(fname)
	return a


def get_favorites():
	url = api_root + 'video/bookmarks/folders.json?per_page=20&access_token=%s' % access_token
	a = get_cached(url, 'folders')
	return a['data']['folders']


def get_bookmarks(folder_id):
	url = api_root + 'video/bookmarks/folders/%d/items.json?per_page=20&access_token=%s' % (folder_id, access_token)
	a = get_cached(url, 'folders')
	return a['data']['bookmarks']


def get_children(child_id, page=1):
	url = api_root + '/video/media/%d/children.json?page=%d&per_page=20&order_by=on_air&access_token=%s' % \
		(child_id, page, access_token)
	a = get_cached(url, 'children')
	return a['data']['children'], a['data']['pagination']


def get_stream_url(object_id):
	url = api_root + 'video/media/%d/watch.json?access_token=%s&format=mp4&protocol=hls&bitrate=%d' % \
		(object_id, access_token, bitrate)
	try:
        	a = get_cached(url, 'stream')
	except:
        	return False, 'exception while getting url' 
	if a['status_code'] != 200:
		return False, 'cannot get video url'
	return True, a['data']['url']


def find_earliest_item(list, last_ids):
	idx = len(list)
	for item in reversed(list):
		if item['id'] not in last_ids:
			return idx - 1
		idx -= 1
	return -1


def print_help():
	s = \
		'        <44> prev page <55> unmark viewed <66> next page\n' + \
		'        <88> settings\n'                + \
		'        <00> quit'

	s = s.replace('<', '\x1b[1;32m')
	s = s.replace('>', '\x1b[0m')
	print s

	
def print_list(list, last_ids, next_idx):
	print '	time:', datetime.datetime.now().strftime('%H:%M')
	print '='*100

	idx = 0
	
	for item in list:
		color = ''
		nocolor = ''
		status = ''

		if next_idx == idx:
			color = '\x1b[1;32m'
			nocolor = '\x1b[0m'
			status = 'NEXT'
		elif item['id'] in last_ids:
			if item['id'] == last_ids[-1]:
				color = '\x1b[1;33m'
				nocolor = '\x1b[0m'
				status = 'last'
			else:
				color = '\x1b[1;31m'
				nocolor = '\x1b[0m'
				status = 'viewed'

		if 'title' in item:
			print '	%s%2d %s %6s%s' % (color, idx+1, item['title'].encode('utf-8').ljust(20), status, nocolor)
		else:
			pos = ''
			if item['id'] in history:
				pos = str(history[item['id']][0]) + 's' 
			print '	%s%2d %s %6s %6s %4d min %s%s' % \
				(color, \
				idx+1, \
				unicode(item['short_name']).ljust(40)[:40].encode('utf-8'), \
				status, pos, item['duration'], item['on_air'].encode('utf-8'), nocolor)
		idx += 1
	print
	print '='*100


def print_location(stack):
	print '	',
	s = ''
	for item in stack:
		s += item.name.encode('utf-8')
		if item.page > 1:
			s += '(' + str(item.page) + ')'
		s += '::'
	if len(s) > 80:
		s = '...' + s[-80:]
	print s,
	print '> ',
	sys.stdout.flush()


def ensure_private_dir(name):
	dname = os.environ['HOME'] + '/' + name
	if not os.path.exists(dname):
		os.mkdir(dname)


def show_settings():

	global bitrate, debug

	while True:
		print '\x1b[2J'
		print '='*100
		print '         access_token              :', access_token
		print '         refresh_token             :', read_param('refresh_token')
		print '	 \x1b[1;32m1\x1b[0m  bitrate [400/800/1200] :', bitrate
		print '	 \x1b[1;32m2\x1b[0m  debug [0/1]            :', debug
		print '='*100
		print '	 press \x1b[1;32mNUMBER\x1b[0m+ENTER to change settings OR just hit ENTER to return'
		print '	 > ',
		cmd = bbctrl.get_command()
		if cmd == '':
			break
		if cmd == '1':
			if bitrate == 400:
				bitrate = 800
			elif bitrate == 800:
				bitrate = 1200
			else:
				bitrate = 400
			write_param('bitrate', str(bitrate))
		elif cmd == '2':
			if debug == 0:
				debug = 1
			else:
				debug = 0
			write_param('debug', str(debug))


class sheet:
	id = 0
	parent = None
	name = 'name'
	category = 'category'
	list = []
	page = 1
	has_next = False
	has_prev = False


def create_sheet_stack():
	s = sheet()
	s.name = 'favorites'
	s.category = 'favorites'
	s.list = get_favorites()
	stack = [s]
	return stack


def stack_bookmarks(id, child):
	sh = sheet()
	sh.id = id
	sh.parent = child
	sh.name = child['title']
	sh.category = 'bookmarks'
	sh.list = get_bookmarks(id)
	stack.append(sh)


def stack_children(id, page, child):
	sh = sheet()
	sh.id = id
	sh.parent = child
	sh.name = child['short_name']
	sh.category = 'children'
	sh.page = page
	sh.list, paging = get_children(id, page)
	sh.has_next = paging['has_next']
	sh.has_prev = paging['has_previous']
	stack.append(sh)


def clear_screen():
	print '\x1b[2J'


def play_video(id):
	clear_screen()

#	print 'playing %d. press ENTER' % id 
#	sys.stdin.readline()
#	return True

	rc, url = get_stream_url(id)
	
	if rc == False:
#		print '\n', url
#		print 'press ENTER to continue'
#		sys.stdin.readline()
		time.sleep(5)
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


ensure_private_dir('.cache')
ensure_private_dir('.cache/etvcc')
ensure_private_dir('.config')
ensure_private_dir('.config/etvcc')

stack = create_sheet_stack()
last_ids = map(lambda x: int(x), read_cached_list('viewed'))
next_idx = -1
selected_idx = -1
history = read_history()
start_player = False
quit = False


def move_next():
	if not stack[-1].has_next:
		return False, None, 0, 0

	child = stack[-1].parent
	id = stack[-1].id
	page = stack[-1].page + 1
	
	return True, child, id, page


def move_prev():
	if not stack[-1].has_prev:
		return False, None, 0, 0

	child = stack[-1].parent
	id = stack[-1].id
	page = stack[-1].page - 1
	
	return True, child, id, page


def unmark_viewed():
	list = stack[-1].list
	for item in list:
		if item['id'] in last_ids:
			last_ids.remove(item['id'])


def process_command(cmd, next_idx):
	global quit
	if cmd == 'quit':
		quit = True
		return True

	if cmd == 'settings':
		show_settings()
		return True

	if cmd == 'unmark':
		unmark_viewed()
		return True

	if cmd == '' and next_idx == -1:
		return True

	return False


def manual_select():
	
	global quit

	cmd = bbctrl.get_command()

	if process_command(cmd, next_idx):
		return True, None, 0, 0

	if cmd in ['prev', 'next']:
		if cmd == 'next':
			moved, child, id, page = move_next()
		elif cmd == 'prev':
			moved, child, id, page = move_prev()

		if not moved:
			return True, None, 0, 0
		return False, child, id, page

	if cmd == '':
		idx = next_idx
	else:
		try:
			idx = int(cmd) - 1
		except:
			return True, None, 0, 0

	if idx == -1:
		stack.pop()
		if len(stack) == 0:
			quit = True
		return True, None, 0, 0
	
	if idx >= len(children):
		return True, None, 0, 0

	child = children[idx]
	id = child['id']
	return False, child, id, 1


while not quit:

	clear_screen()

	state = stack[-1].category
	children = stack[-1].list
	page = stack[-1].page 

	next_idx = find_earliest_item(stack[-1].list, last_ids)

	if start_player and next_idx != -1:
		child = children[next_idx]
		id = child['id']
	else:
		print_list(stack[-1].list, last_ids, next_idx)
		print_help()
		print_location(stack)
		done, child, id, page = manual_select()

		if done:
			continue

		if state == 'favorites':
			stack_bookmarks(id, child)
		elif state == 'bookmarks' or state == 'children':
			if child['type'] == 'Container':
				stack_children(id, page, child)
			else:
				start_player = True

	if not start_player:
		continue

	start_time = time.time()
	
	rc =  play_video(id)

	if rc == 1:
		start_player = False
		continue

	append_cached('viewed', id)
	last_ids.append(id)
	played_time = int(time.time() - start_time)
	history[id] = [played_time]
	add_history(id, played_time)
	start_player = (rc == 2) or (played_time > (child['duration'] * 60 - 120))
