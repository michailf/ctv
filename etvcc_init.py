#!/usr/bin/python

# activate etvnet on local profile

import urllib, urllib2, json, sys, os

code_url = 'https://accounts.etvnet.com/auth/oauth/device/code'
token_url = 'https://accounts.etvnet.com/auth/oauth/token'
scope = 'com.etvnet.media.browse com.etvnet.media.watch com.etvnet.media.bookmarks com.etvnet.media.history \
com.etvnet.media.live com.etvnet.media.fivestar com.etvnet.media.comments com.etvnet.persons com.etvnet.notifications'

def get_code():

    data = {
        'client_id': 'a332b9d61df7254dffdc81a260373f25592c94c9',
        'client_secret': '744a52aff20ec13f53bcfd705fc4b79195265497',
        'scope': scope
    }

    udata = urllib.urlencode(data)
    req = urllib2.Request(code_url)
    resp = urllib2.urlopen(req, udata).read()

    a = json.loads(resp)
    print '\n\n\n'
    print '    go to etvnet.com website and enter the activation code:', a['user_code']
    print '    then select number and press ENTER:'
    print
    print '    1     confirm'
    print '    0     cancel'
    print
    print '    >',
    cmd = sys.stdin.readline().strip()
    if cmd != '1':
        print '    canceled'
        sys.exit(1)

    return a['device_code']

def write_param(name, value):
    f = open(os.environ['HOME'] + '/.config/etvcc/' + name + '.txt', 'wt')
    f.write(value)
    f.close()

def ensure_private_dir(name):
    dname = os.environ['HOME'] + '/' + name
    if not os.path.exists(dname):
        os.mkdir(dname)

def get_token(device_code):

    data = {
        'client_id': 'a332b9d61df7254dffdc81a260373f25592c94c9',
        'client_secret': '744a52aff20ec13f53bcfd705fc4b79195265497',
        'scope': scope,
        'grant_type': 'http://oauth.net/grant_type/device/1.0',
        'code': device_code
    }

    udata = urllib.urlencode(data)
    req = urllib2.Request(token_url)
    resp = urllib2.urlopen(req, udata).read()
    a = json.loads(resp)
    error = a.get('error')
    if error:
        print '   ERROR:', error
        print '   ', resp
        sys.exit(1)
    
    print '\n\n\n'
    print '    access_token: ', a['access_token']
    print '    refresh_token:', a['refresh_token']
    print '\n\n\n'
    print '    save to etvcc profile?'
    print '    1     confirm'
    print '    0     cancel'
    print
    print '    >'
    cmd = sys.stdin.readline().strip()
    if cmd != '1':
        print '    canceled'
        sys.exit(1)

    ensure_private_dir('.cache')
    ensure_private_dir('.cache/etvcc')
    ensure_private_dir('.config')
    ensure_private_dir('.config/etvcc')

    write_param('access_token', a['access_token'])
    write_param('refresh_token', a['refresh_token'])
    print '    activated'

device_code = get_code()
get_token(device_code)
