# Taken from KODI

import re
import urllib
import urllib2

def fetch(url, fname):
	response = urllib2.urlopen(url)
	html = response.read()
	f = open(fname, 'wt')
	f.write(html)
	f.close()
	return html

html = fetch('http://www.smithsonianchannel.com/full-episodes', 'main.txt')
a = re.compile('data-premium=".+?href="(.+?)".+?srcset="(.+?)".+?"timecode">(.+?)<.+?</li>',re.DOTALL).findall(html)

for i, (url, thumb, dur) in list(enumerate(a, start=1))[:10]:
	html = fetch('http://www.smithsonianchannel.com%s' % url, 'f1-%d.txt' % i)

	m = re.compile('property="og:title" content="(.+?)"', re.DOTALL).search(html)
	title = m.group(1)

	html = fetch('http://www.smithsonianchannel.com%s' % urllib.unquote_plus(url), 'f2-%d.txt' % i)
	(suburl, vidID) = re.compile('data-vttfile="(.+?)".+?data-bcid="(.+?)"', re.DOTALL).search(html).groups()
	url = 'http://c.brightcove.com/services/mobile/streaming/index/master.m3u8?videoId=%s&pubId=1466806621001' % (vidID)

	print i, title
	print '    dur:', dur.replace(' | ', '')
	print '    url:', url
