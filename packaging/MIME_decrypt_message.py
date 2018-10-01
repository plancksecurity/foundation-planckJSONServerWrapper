#!/usr/bin/env python

from __future__ import unicode_literals, print_function

from testbase import *

import sys
import os

func_callid = 0

def _json(s, fn_name, params):
    global func_callid

    d, env, p, token = s.dj, s.env, s.process, s.token, 

    # fn_name = "MIME_decrypt_message"
    # params = ['', 0, ["OP"],["OP"],["OP"],["OP"]]
    # params[0] = msg
    # params[1] = len(msg)
    func_callid += 1
    func = {"id": func_callid,
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
    buf = io.StringIO(resp.content.decode('utf8'))
    ret = json.load(buf)
    print(json.dumps(ret))

    result = ret['result']['return']
    return result


def myself(s, email):
    params = [{
      "user_id": "pEp_own_userId",
      "username": email,
      "address": email,
      "fpr": ""
    }]
    return _json(s, "myself", params)
    

def MIME_decrypt_message(s, msg):
    params = ['', 0, ["OP"],["OP"],["OP"],["OP"]]
    params[0] = msg
    params[1] = 0
    return _json(s, "MIME_decrypt_message", params)
    

def main():
    try:
        sys.argv[2]
    except:
        print("Usage: MIME_decrypt_message.py <pep-json-server-binary> <email.msg> [<myself-email>]")
        return 1

    class dj(object):    
        home = os.environ.get('HOME')
        gnupg = os.environ.get('GNUPGHOME', os.path.expanduser('~/.gnupg'))
        pep = os.path.expanduser('~/.pEp')
        lappdat = os.environ.get('HOME') + '/.local'
        temp = os.environ.get('TMPDIR')

    with test_server(sys.argv[1], dj=dj) as s:
        try:
            myself(s, sys.argv[3])
        except:
            pass
        with open(sys.argv[2], 'r') as fh:
            #msg = p.parse(fh, headersonly=True)
            msg = fh.read()
            MIME_decrypt_message(s, msg)


if __name__ == "__main__":
    sys.exit(main())
