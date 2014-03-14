#!/usr/bin/python

import struct, sys, os, select, time

readers = []
FORMAT = 'llHHI' # i64, i64, u16, u16, i32
EVENT_SIZE = struct.calcsize(FORMAT)

rfd, wfd = os.pipe()
wpipe = os.fdopen(wfd, 'wt')
rpipe = os.fdopen(rfd, 'rt')
log = open('ir.log', 'wt')

def init():
	i = 0
	while True:
		fname = '/dev/input/event' + str(i)
		i += 1
		if os.path.exists(fname):
			fd = open(fname, 'rb')
			readers.append(fd)
		else:
			break


def get_worker_fds():
	return readers


def get_event_fd():
	return rpipe


def dispatch_event(r):
	event = r.read(EVENT_SIZE)
	(tv_sec, tv_usec, type, code, value) = struct.unpack(FORMAT, event)
	print >>log, "Event type %u, code %u, value: %u at %d, %d" % \
		(type, code, value, tv_sec, tv_usec)
	if type == 1 and value == 1 and code in [106, 105, 103, 108]:
		c = 'ir_' + str(code) + '\n'
		wpipe.write(c)
		wpipe.flush()


def close():
	for r in readers:
		r.close()

init()

def main():
	while True:
		code = wait()
		print code

if __name__ == '__main__':
	main()
