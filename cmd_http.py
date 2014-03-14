import sys, select, os, subprocess, re
import BaseHTTPServer

def get_ipv4_address():
	p = subprocess.Popen(["/sbin/ifconfig"], stdout=subprocess.PIPE)
	ifc_resp = p.communicate()
	patt = re.compile(r'inet\s*\w*\S*:\s*(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})')
	resp = patt.findall(ifc_resp[0])
	return resp[0]

port = 44125
	
html = '''
<html>
<head>
<script>

var xmlHttp = new XMLHttpRequest();

function send(k)
{
	xmlHttp.open("GET", 'http://%s:%d?key=' + k, false);
	xmlHttp.send(null);
	var el = document.getElementById('key');
	el.innerHTML = 'resp:' + xmlHttp.responseText;
}

function load()
{
	 document.addEventListener('keydown', function(event) {
		send(event.keyCode);
	});
}
load();
</script>
</head>
<body>
<div id="key"></div>
<pre>
</pre>
</body>
</html>
''' % (get_ipv4_address(), port)

# blackberry keys
# w e r
# s d f
# z x c
#   0
bb_numkeys = { 87:1, 69:2, 82:3,  83:4, 68:5, 70:6, 90:7, 88:8, 67:9, 48:0 }
lastkey = 0
cmdline = ''

class http_handler(BaseHTTPServer.BaseHTTPRequestHandler):

	def do_GET(s):
		global lastkey
		global cmdline
		if s.path.startswith('/?key='):
			c = s.path.split('=')
			lastkey = int(c[1])
			if lastkey in bb_numkeys.keys():
				lastkey = bb_numkeys[lastkey]
				cmdline += str(lastkey)

			resp = cmdline
			s.wfile.write('HTTP/1.1 200 OK\r\n')
			s.wfile.write('Content-Type: text/plain\r\n')
			s.wfile.write('Content-Length: %s\r\n\r\n' % len(resp))
			s.wfile.write(resp)
			return
			
		resp = html
		s.wfile.write('HTTP/1.1 200 OK\r\n')
		s.wfile.write('Content-Type: text/html\r\n')
		s.wfile.write('Content-Length: %s\r\n\r\n' % len(resp))
		s.wfile.write(resp)

	def address_string(self):
		return 'address_string'

server_address = ('', port)
httpd = BaseHTTPServer.HTTPServer(server_address, http_handler)
rfd, wfd = os.pipe()
wpipe = os.fdopen(wfd, 'wt')
rpipe = os.fdopen(rfd, 'rt')


def get_worker_fd():
	return httpd


def get_event_fd():
	return rpipe


def dispatch_event():
	global cmdline, lastkey
	httpd.handle_request()
	if lastkey == 13:
		c = cmdline + '\n'
		cmdline = ''
		lastkey = 0
		wpipe.write(c)
		wpipe.flush()


def run():
	httpd_worker = get_worker_fd()
	httpd_event = get_event_fd()
	
	readers = [sys.stdin, httpd_worker, httpd_event]
	
	while True:
		dr, dw, de = select.select(readers, [], [], 30.0)
		
		if sys.stdin in dr:
			print 'exit'
			break
		elif httpd_worker in dr:
			dispatch_event()
		elif httpd_event in dr:
			cmd = httpd_event.readline().strip()
			print 'httpd_event:' + cmd
			if cmd == '00':
				break
		elif len(dr) == 0:
			continue
		else:
			print 'unknown event. fd: ' + str(dr)

if __name__ == "__main__":
	print 'starting on ', server_address
	run()
