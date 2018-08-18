#!/usr/bin/env python

from __future__ import unicode_literals, print_function

from testbase import *

import sys
import os



def get_version(d, env, p, token):

    fn_name = "serverVersion"
    params = []
    func = {"id": 1,
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

    try:
        result["package_version"] = sys.argv[3]
    except IndexError:
        print("package_version not fiven [arg3]")

    # if 'errorstrack' in result:
    #     del result['errorstrack']
    if len(sys.argv) > 2:
        with open(sys.argv[2], 'wx') as fh:
            fh.write(json.dumps(result, ensure_ascii=False).encode('utf8'))
            fh.write(u"\n")
    

def main():
    with test_server(sys.argv[1]) as s:
        get_version(s.dj, s.env, s.process, s.token)


if __name__ == "__main__":
    sys.exit(main())
