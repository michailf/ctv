import sys
import tty
import termios
import os
import time

fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(fd)

DIR = os.environ['HOME'] + '/.cache/etvcc/'
MAX_ITEMS = 40

def read_int(fname, def_num=1):
	fn = DIR + fname
	if not os.path.exists(fn):
		return def_num
	f = open(fn);
	t = int(f.read())
	f.close()
	return t

curr = read_int('curr', 0)

def write_int(fname, num):
	f = open(DIR + fname, 'wt');
	f.write(str(num))
	f.close()

def clear_screen():
	print '\x1b[2J'

def print_list(list):
	print '='*60
	cnt = 0
	for item in list:
		m1 = ' '
		m2 = ' '
		if curr == cnt:
			m1 = '\x1b[1;33m>'
			m2 = '<\x1b[0m'

		if item['type'] == 'MediaObject':
			num = ' - '
		else:
			cnum = read_int('%d.num' % item['id'])
			num = '%d/%d' % (cnum, item['children_count'])

		tmstr = item['on_air'].encode('utf-8')
		print '%s %2d %s %s %s %s' % \
			(m1, \
			cnt + 1, \
			unicode(item['short_name']).ljust(40)[:40].encode('utf-8'), \
			num.ljust(7), \
			tmstr, \
			m2)
		cnt += 1
	if len(list) < 20:
		print '\n'*(20-len(list)-1)

def getch():
	tty.setraw(sys.stdin.fileno())
	ch = sys.stdin.read(1)
	termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
	return ch

def move(list, ch):
	global curr
	size = len(list)

	if ch == 'B':
		curr = (curr + 1) % size
		write_int('curr', curr)
	elif ch == 'A':
		curr = (curr - 1) % size
		write_int('curr', curr)
	elif ch in [ 'C', 'D' ]:
		max_num = list[curr]['children_count']
		if max_num > 0:
			num = read_int('%d.num' % list[curr]['id'])
			if ch == 'C':
				num = (num % max_num) + 1
			else:
				num = ((num - 2) % max_num) + 1
			write_int('%d.num' % list[curr]['id'], num)

def loop(list):
	while True:
		print_list(list)
		ch = getch()
		if ch in [ '0', 'q' ] :
			break
		move(list, ch)

#termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
