import os

def read_param(name, default = None):
	fname = os.environ['HOME'] + '/.config/etvcc/' + name + '.txt'

	if not os.path.exists(fname) and default is not None:
		return default

	f = open(fname)
	s = f.readline().strip()
	f.close()
	return s

def write_param(name, value):
	f = open(os.environ['HOME'] + '/.config/etvcc/' + name + '.txt', 'wt')
	f.write(value + '\n')
	f.close()

def ensure_private_dir(name):
	dname = os.environ['HOME'] + '/' + name
	if not os.path.exists(dname):
		os.mkdir(dname)