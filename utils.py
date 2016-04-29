import os

config_dir = os.environ['HOME'] + '/.config/etvcc/'

def read_param(name, default = None):
	fname = config_dir + name + '.txt'

	if not os.path.exists(fname):
		if default is not None:
			return default
		else:
			raise BaseException('cannot read parameter from config: ' + name)

	f = open(fname)
	s = f.readline().strip()
	f.close()
	return s

def write_param(name, value):
	f = open(config_dir + name + '.txt', 'wt')
	f.write(value + '\n')
	f.close()

def ensure_private_dir(name):
	dname = os.environ['HOME'] + '/' + name
	if not os.path.exists(dname):
		os.mkdir(dname)
