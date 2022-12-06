#!/usr/bin/env python3

import requests
import json
import pprint as pp


def server_info() -> (str, str):
    json_token = open('../server/.pEp/json-token')
    json_server = json.load(json_token)
    url = 'http://{}:{}{}'.format(json_server['address'], json_server['port'], json_server['path'])
    rpc_url = url + 'callFunction'
    return (rpc_url, json_server['security_token'])


def json_rpc_request(func_name: str, params: dict) -> dict:
    params_val = []
    if len(params) > 0:
        params_val = [params]


    request = {
        'id': 1001,
        'jsonrpc': '2.0',
        'security_token': server_info()[1],
        'method': func_name,
        'params': params_val}
    pp.pprint('REQUEST:')
    pp.pprint(request)
    return request


def generic(*args, **kwargs):
    print("-------------------------------------------------------")
    function_name: str = args[0]
    params: dict = kwargs
    request: dict = json_rpc_request(function_name, params)

    pp.pprint('RESPONSE:')
    response: requests.Response = requests.post(server_info()[0], json=request)
    pp.pprint(response.json())


def main():
    print(server_info()[0])
    print(server_info()[1])


if __name__ == '__main__':
    main()
    generic('myself', username='alice', address='alice@peptest.org')
    generic('startSync')
    # generic('stopSync')
