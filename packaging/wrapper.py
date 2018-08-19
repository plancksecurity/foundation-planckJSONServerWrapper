#!/usr/bin/env python

from __future__ import unicode_literals, print_function

from testbase import *

import sys
import os
import json

import requests

from flask import Flask, request
from flask import Response
from flask import stream_with_context

from collections import OrderedDict

# from werkzeug.serving import BaseWSGIServer, WSGIRequestHandler
# WSGIRequestHandler.protocol_version = "HTTP/1.1"

import logging
# try:
import http.client as http_client

from pprint import pprint

# except ImportError:
#     # Python 2
#     import httplib as http_client
http_client.HTTPConnection.debuglevel = 1

# You must initialize logging, otherwise you'll not see debug output.
logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True


app = Flask(__name__)

@app.route('/flask.html')
def hello_world():
    return 'Hello, World!'

# @app.route('/', defaults={'path': ''})
# @app.route('/<path:path>')
# def home(path):
#     json_url = "http://127.0.0.1:4223/" + path
#     req = requests.get(json_url, stream=True)
#     return Response(stream_with_context(req.iter_content()), content_type = req.headers['content-type'])



@app.before_request
def before():
    print(">>>>> REQUEST")
    print(request.headers)
    # print(request.data)

@app.after_request
def after(response):
    print("----- RESPONSE")
    # todo with response
    print(response.status)
    print(response.headers)
    print("<<<<<< RESPONSE")
    # print(response.get_data())
    if 'Connection' not in response.headers:
        if request.headers.get('connection', 'close') == 'keep-alive':
            response.headers['Connection'] = 'keep-alive'
    return response



# import urllib3
import requests.adapters

adapter_sessions = {}
gToken = {}

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
# @app.route('/ja/0.1/callFunction', methods=['GET', 'POST'])
def callFunction(path):

    sess_key = "{0}:{1}".format(
        request.environ.get('REMOTE_HOST'),
        request.environ.get('REMOTE_PORT')
        )

    # Precedence: Origin, Referer, Host
    # Format: host[:port]
    hostport = request.headers.get('Host', '0.0.0.0:80')
    # Format: URL (scheme://host[:port])
    orig = request.headers.get('Referer', 'http://' + hostport)
    orig = request.headers.get('Origin', orig)
    #
    orig = orig.split('//', 1)[-1]
    host = hostport.split(':', 1)[0]

    sess = adapter_sessions.get(sess_key)
    if not sess:
        sess = requests.Session()
        sess.mount('http://', requests.adapters.HTTPAdapter(pool_connections=1))
        adapter_sessions[sess_key] = sess

    token = dict(gToken)
    token['address'] = host
    json_url = "http://{address:s}:{port:d}/{lpath:s}".format(lpath=path, **token)
    kw = dict()

    data = request.get_data()
    data = json.loads( data ) if data else None
    if data is not None and data.get('security_token', None) != token['security_token']:
        for x in ('localhost', '127.0.0.1', '[::1]'):
            if orig.startswith(x + ':') or orig == x:
                print("Warning: security token does not match (substituting!)")
                print('')
                data['security_token'] = token['security_token']
                break
            
    h = dict(request.headers)
    h['Host'] = "{address:s}:{port:d}".format(**token)
    d = json.dumps( data ) if data is not None else None
    req = requests.Request(request.method, json_url,
        headers=h, data=d)
        # **kw)
    r = req.prepare()
    resp = sess.send(r, stream=True)

    resp.headers['Connection'] = req.headers.get('Connection', 'close')
    
    if request.method in ('POST',):
        resp.headers['Access-Control-Allow-Origin'] = 'http://' + orig

    return Response(resp.content,
                headers=OrderedDict(resp.headers),
                content_type=resp.headers['content-type'])


def pephome(f=None):
    f = f if f else ''
    h = os.environ.get("PEPHOME", None)
    if h is not None:
        return os.path.join(h, f)
    h = os.environ.get("HOME", os.environ.get("APPDATA", None))
    if h is None:
        raise Exception("PEPHOME")
    # TODO: use pEp not .pEp on Windows
    return os.path.join(h, ".pEp", f)


def run_main(d, env, p, token):
    global gToken
    gToken = token

    with open(pephome("json-token.orig"),  "w") as fh:
        fh.write(json.dumps(token))

    tk2 = dict(token)
    tk2['port'] = port = 5000
    with open(pephome("json-token"),  "w") as fh:
        fh.write(json.dumps(tk2))

    # Allow persistent connections (Connection: keep-alive)
    from werkzeug.serving import WSGIRequestHandler
    WSGIRequestHandler.protocol_version = "HTTP/1.1"

    app.run(port=port, debug=True, use_reloader=False, threaded=True)


def main():
    try:
        json_bin = sys.argv[1]
    except:
        json_bin = "/Library/Frameworks/pEpDesktopAdapter.framework/Versions/A/bin/pep-json-server"
    with test_server(json_bin) as s:
        # os.environ["PEPHOME"] = s.pep
        #print("PEPHOME=" + s.pep)

        run_main(s.dj, s.env, s.process, s.token)


if __name__ == "__main__":
    sys.exit(main())
