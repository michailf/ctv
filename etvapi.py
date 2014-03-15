import json, os, urllib, urllib2, shutil, hashlib
import utils

# ==== constants ===================================
client_id = "a332b9d61df7254dffdc81a260373f25592c94c9"
client_secret = '744a52aff20ec13f53bcfd705fc4b79195265497'
oauth_url = 'https://accounts.etvnet.com/auth/oauth/token'
api_root = 'https://secure.etvnet.com/api/v3.0/'
scope = 'com.etvnet.media.browse com.etvnet.media.watch com.etvnet.media.bookmarks com.etvnet.media.history \
com.etvnet.media.live com.etvnet.media.fivestar com.etvnet.media.comments com.etvnet.persons com.etvnet.notifications'
# ==================================================

# ==== params =====================================
access_token = utils.read_param('access_token')
debug = False
bitrate = 1200
# ==================================================

def read_json(fname):
	f = open(fname)
	text = f.read()
	f.close()
	a = json.loads(text)
	return a

def refresh_token():
	global access_token

	data = {
		'client_id': client_id,
		'client_secret': client_secret,
		'grant_type': 'refresh_token',
		'refresh_token': utils.read_param('refresh_token'),
		'scope': scope
		}

	udata = urllib.urlencode(data)
	req = urllib2.Request(oauth_url)
	resp = urllib2.urlopen(req, udata).read()
	if debug:
		print 'refresh_token resp:', resp
	a = json.loads(resp)
	utils.write_param('access_token', a['access_token'])
	utils.write_param('refresh_token', a['refresh_token'])
	access_token = utils.read_param('access_token')

def get_cached(url, name):
	md5 = hashlib.md5(url).hexdigest()
	fname = os.environ['HOME'] + '/.cache/etvcc/' + name + '-' + md5 + '.json'
	
	if not os.path.exists(fname):
		tmpname = ''
		try:
			turl = url + '&access_token=' + access_token
			(tmpname, headers) = urllib.urlretrieve(turl)
		except:
			refresh_token()
			turl = url + '&access_token=' + access_token
			(tmpname, headers) = urllib.urlretrieve(turl)
				
		shutil.move(tmpname, fname)
	a = read_json(fname)
	return a


def get_favorites():
	url = api_root + 'video/bookmarks/folders.json?per_page=20'
	a = get_cached(url, 'folders')
	return a['data']['folders']

def get_bookmarks(folder_id):
	url = api_root + 'video/bookmarks/folders/%d/items.json?per_page=20' % (folder_id)
	a = get_cached(url, 'folders')
	return a['data']['bookmarks']

def get_stream_url(object_id):
	url = api_root + 'video/media/%d/watch.json?format=mp4&protocol=hls&bitrate=%d' % \
		(object_id, bitrate)
	try:
        	a = get_cached(url, 'stream')
	except:
        	return False, 'exception while getting url' 
	if a['status_code'] != 200:
		return False, 'cannot get video url'
	return True, a['data']['url']

def get_children(child_id, page):
	url = api_root + '/video/media/%d/children.json?page=%d&per_page=20&order_by=on_air' % \
		(child_id, page)
	a = get_cached(url, 'children')
	return a['data']['children']
