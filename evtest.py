#!/usr/bin/python

import struct
import time
import sys
import select

infile_path = "/dev/input/event" + (sys.argv[1] if len(sys.argv) > 1 else "0")

#long int, long int, unsigned short, unsigned short, unsigned int
FORMAT = 'llHHI'
EVENT_SIZE = struct.calcsize(FORMAT)

#open file in binary mode
in_file = open(infile_path, "rb")

readers = [in_file]

while True:
	dr, dw, de = select.select(readers, [], [], 1.0)
#	print 'dr: ' + str(dr)

	if in_file in readers:
		event = in_file.read(EVENT_SIZE)
		(tv_sec, tv_usec, type, code, value) = struct.unpack(FORMAT, event)
	
		if type == 1 and value == 1:
			print 'ir_' + str(code)
#		print("Event type %u, code %u, value: %u at %d, %d" % \
#			(type, code, value, tv_sec, tv_usec))
#	else:
#		print 'unknown event: ' + str(dr)

in_file.close()
