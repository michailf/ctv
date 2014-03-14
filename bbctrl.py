import sys, select
import cmd_http
import cmd_ir

httpd_worker = cmd_http.get_worker_fd()
httpd_event = cmd_http.get_event_fd()
ir_workers = cmd_ir.get_worker_fds()
ir_event = cmd_ir.get_event_fd()

readers = [sys.stdin, httpd_worker, httpd_event, ir_event]
for wrk in ir_workers:
	readers.append(wrk)

def get_command():
	done = False
	cmdline = ''
	cmd = ''
	
	while True:

		if done:
			if cmdline in ['q', 'x', '00']:
				cmd = 'quit'
			elif cmdline in ['p', '44', 'ir_105']:
				cmd = 'prev'
			elif cmdline in ['n', '66', 'ir_106']:
				cmd = 'next'
			elif cmdline in ['88']:
				cmd = 'settings'
			elif cmdline in ['103']:
				cmd = 'next_movie'
			elif cmdline in ['108']:
				cmd = 'prev_movie'
			elif cmdline in ['55']:
				cmd = 'unmark'
			else:
				cmd = cmdline
			return cmd

		dr, dw, de = select.select(readers, [], [], 1.0)
#		print 'dr: ' + str(dr)
		
		if sys.stdin in dr:
			cmdline = sys.stdin.readline().strip()
			done = True
			continue

		wrk_done = False
		for wrk in ir_workers:
			if wrk in dr:
				cmd_ir.dispatch_event(wrk)
				wrk_done = True
				break
		
		if wrk_done:
			continue
			
		if httpd_worker in dr:
			cmd_http.dispatch_event()
			continue

		if ir_event in dr:
			cmdline = ir_event.readline().strip()
			done = True
			continue

		if httpd_event in dr:
			cmdline = httpd_event.readline().strip()
			done = True
			continue

		if len(dr) == 0:
			continue

		print 'unknown event. fd: ' + str(dr)


def get_command_prev():
	global cmdline
	cmdline = ''
	readers = [sys.stdin, httpd]
	done = False

	while True:
		dr, dw, de = select.select(readers, [], [], 30.0)
		if httpd in dr:
			httpd.handle_request()
			if lastkey == 13:
				done = True
		elif sys.stdin in dr:
			cmdline = sys.stdin.readline().strip()
			done = True
		
		if done:
			if cmdline in ['q', 'x', '00']:
				cmdline = 'quit'
			elif cmdline in ['p', '44']:
				cmdline = 'prev'
			elif cmdline in ['n', '66']:
				cmdline = 'next'
			elif cmdline in ['88']:
				cmdline = 'settings'
			elif cmdline in ['55']:
				cmdline = 'unmark'
			return cmdline


def run():
	while True:
		cmd = get_command()
		print cmd
		if cmd == '00':
			break


if __name__ == "__main__":
	print 'starting on ', server_address
	run()
