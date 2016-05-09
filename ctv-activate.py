#!/usr/bin/python

# activate console etvnet player (ctv) on etvnet.com
# Instructions:
# - run ctv-activate.py.
# - copy activation code.
# - press 1 to save the code locally.
# - go to etvnet.com website to 'Menu/Activate STB' and enter the code.
# - put some movies into favorites groups on etvnet.com.
# - run ctv.py to load your favorites and view movies.

import urllib, urllib2, json, sys, os, etvapi, utils

def get_code():

    data = {
        'client_id': etvapi.client_id,
        'client_secret': etvapi.client_secret,
        'scope': etvapi.scope
    }

    udata = urllib.urlencode(data)
    req = urllib2.Request(etvapi.code_url)
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

def get_token(device_code):

    data = {
        'client_id': etvapi.client_id,
        'client_secret': etvapi.client_secret,
        'scope': etvapi.scope,
        'grant_type': 'http://oauth.net/grant_type/device/1.0',
        'code': device_code
    }

    udata = urllib.urlencode(data)
    req = urllib2.Request(etvapi.token_url)
    resp = urllib2.urlopen(req, udata).read()
    print 'token_url:', etvapi.token_url
    print 'udata:', udata
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
    print '    save to ctv profile?'
    print '    1     confirm'
    print '    0     cancel'
    print
    print '    >'
    cmd = sys.stdin.readline().strip()
    if cmd != '1':
        print '    canceled'
        sys.exit(1)

    utils.ensure_private_dir('.cache')
    utils.ensure_private_dir('.cache/etvcc')
    utils.ensure_private_dir('.config')
    utils.ensure_private_dir('.config/etvcc')

    utils.write_param('access_token', a['access_token'])
    utils.write_param('refresh_token', a['refresh_token'])
    print '    activated'

device_code = get_code()
get_token(device_code)
