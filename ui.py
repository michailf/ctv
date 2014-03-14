import sys
import tty
import termios
import os
import time

fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(fd)

LISTDIR = '.cache/list/'
MAX_ITEMS = 40

def read_int(fname, def_num=1):
	fn = LISTDIR + fname
	if not os.path.exists(fn):
		return def_num
	f = open(fn);
	t = int(f.read())
	f.close()
	return t

curr = read_int('curr', 0)

def write_int(fname, num):
	f = open(LISTDIR + fname, 'wt');
	f.write(str(num))
	f.close()

def get_time(dirname):
	tm = os.path.getmtime(LISTDIR + dirname)
	tmstr = time.strftime('%Y-%m-%d', time.localtime(tm))
	return tmstr

def print_list(list):
	print '\n'*40
	print '='*100
	cnt = 0
	for dirname in list:
		m1 = ' '
		m2 = ' '
		if curr == cnt:
			m1 = '\x1b[1;33m>'
			m2 = '<\x1b[0m'

		tmstr = get_time(dirname)
		num = read_int(dirname + '/num')
		
		print '%s %-20s %2d   %s %s' % (m1, dirname[:20], num, tmstr, m2)
		cnt += 1

def getch():
	tty.setraw(sys.stdin.fileno())
	ch = sys.stdin.read(1)
	termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
	return ch

def move(list, ch):
	global curr
	if ch == 'B':
		curr = (curr + 1) % MAX_ITEMS
		write_int('curr', curr)
	elif ch == 'A':
		curr = (curr - 1) % MAX_ITEMS
		write_int('curr', curr)
	elif ch == 'C':
		max_num = 20
		num = read_int('%s/num' % list[curr])
		num = (num + 1) % max_num
		write_int('%s/num' % list[curr], num)
	elif ch == 'D':
		max_num = 20
		num = read_int('%s/num' % list[curr])
		num = (num - 1) % max_num
		write_int('%s/num' % list[curr], num)

def loop():
	list = os.listdir(LISTDIR)[:MAX_ITEMS]
	while True:
		print_list(list)
		ch = getch()
		if ch in [ '0', 'q' ] :
			break
		move(list, ch)

try:
	loop()
finally:
	termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
	print 'tty mode restored'
