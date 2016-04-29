import sys
import tty
import termios
import os
import time

fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(fd)

DIR = os.environ['HOME'] + '/.cache/etvcc/'
PAGE_SIZE = 20

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
	print
	cnt = 0
	s = ''
	for item in list:
		m1 = ' '
		m2 = ' '
		if curr == cnt:
			m1 = '\x1b[1;33m*'
			m2 = '\x1b[0m'

		if item['type'] == 'MediaObject':
			num = ' - '
		else:
			cnum = read_int('%d.num' % item['id'])
			num = '%d/%d' % (cnum, item['children_count'])

		tmstr = item['on_air'].encode('utf-8')
		s += '%s %2d %s %s %s %s\n' % \
			(m1, \
			cnt + 1, \
			unicode(item['short_name']).ljust(40)[:40].encode('utf-8'), \
			num.ljust(7), \
			tmstr, \
			m2)
		cnt += 1
	if len(list) < PAGE_SIZE:
		s += '\n'*(PAGE_SIZE - len(list))
	print s

def getch():
	while True:
		tty.setraw(sys.stdin.fileno())
		ch = sys.stdin.read(1)
		termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
		if ch in ['A', 'B', 'C', 'D', 'q', '0', '\r', '1', '2', '3', '4']:
			break
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
