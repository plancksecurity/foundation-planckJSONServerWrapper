#!/usr/bin/env python

import sys
import os
import subprocess
from subprocess import Popen, PIPE
from tempfile import mkdtemp
import shutil
import time
import json
import urllib2
import io
import requests
import pprint

jj = os.path.join
dj = None

def read_token(d, env):
    try:
        with open(os.path.join(d.pep, "json-token"), 'r') as fd:
            token = json.load(fd)
        return token
    except IOError as e:
        if e.errno != 2:
            raise


global_req_id = 0

def basic_authorization(user, password):
    s = user + ":" + password
    return "Basic " + s.encode("base64").rstrip()


def run_tests(d, env, p, token):

    global global_req_id

    # global_req_id += 1

    for fn_name in ("version", "serverVersion", "getGpgEnvironment"):

        global_req_id += 1

        params = []
        func = {"id": global_req_id,
              "jsonrpc": "2.0",
              "security_token": token['security_token'],
              "method": fn_name,
              "params": params,
            }
        url = agent_url(token)

        resp = requests.post(url + "callFunction", json=func)
        if resp.status_code != 200:
            # print(resp.errorstack)
            raise ValueError(resp.status_code)
        ret = json.load(io.StringIO(resp.content.decode('utf8')))
        pprint.pprint(ret)



def agent_url(token):
    return "http://%s:%s%s" % (
        token.get("address", "127.0.0.1"),
        token.get("port", "0"),
        token.get("path", "/ja/0.1/"),
    )


def sec_token(token):
    return token["security_token"]


def main():
    with test_server(sys.argv[1]) as s:
        run_tests(s.dj, s.env, s.process, s.token)


class test_server(object):

    m = None

    def __init__(self, server_bin, env=None):
        self.server_bin = server_bin
        self.m = m = mkdtemp(prefix="testjson")
        self.env = env if env else {}
        class dj(object):    
            home = os.path.join(m, "home")
            gnupg = os.path.join(m, "home", ".gnupg")
            pep = os.path.join(m, "home", ".pEp")
            lappdat = os.path.join(m, "home", "Data", "Local")
            temp = os.path.join(m, "tmp")
        self.dj = dj

        for d in dj.__dict__:
            if d.startswith('_'):
                continue
            try:
                os.makedirs(getattr(dj, d))
            except OSError:
                pass

        shutil.copyfile(os.path.expanduser("~/.pEp_management.db"),
                        jj(dj.home, ".pEp_management.db"))


    def __enter__(self):
        server_bin = self.server_bin
        m = self.m
        dj = self.dj

        server_sharedir = os.path.abspath(
                os.path.join(os.path.dirname(server_bin),
                             "..", "share", "pEp"))

        env = dict( (k, os.environ[k]) for k in "PATH USER".split() )
        # env['PATH'] = "/usr/bin:/bin:/usr/sbin:/sbin"
        env.update({
            'LOCALAPPDATA': dj.lappdat,
            'HOME': dj.home,
            'GNUPGHOME': dj.gnupg,
            'TEMP': dj.temp, 'TMP': dj.temp, 'TMPDIR': dj.temp,
        })
        env.update(self.env)
        self.env = env

        self.process = Popen([server_bin,
                # "-l", "stderr",
                "-d", "1"], executable=server_bin,
                            shell=False, cwd=server_sharedir, env=env,
                            stdin=PIPE)
        
        # Do what we would not have to do if we could run the server in daemon mode.
        # Server guarantees that token file is written only after server is ready (SHOULD!)
        token = None
        for x in range(0, 10):
            token = read_token(dj, env)
            if token:
                break
            time.sleep(.125)    
        if not token:
            raise Exception("Fail: no token")

        self.token = token

        return self


    def __exit__(self, a, b, c):
        p = self.process
        p.stdin.write("q")
        p.stdin.write("\n")
        # process.stdin.close()
        ret = p.wait()

        if self.m is not None:
            shutil.rmtree(self.m)
        if a is not None:
            return False
        if ret != 0:
            raise Exception("return code %d" % ret)
        return False


if __name__ == "__main__":
    sys.exit(main())
