# p≡p JSON Server Adapter

## Introduction

The p≡p JSON Server Adapter provides a REST-like jQuery-compatible API to connect with
the p≡p engine.  It is language-independent and can be used by any client.

## Getting started - build and run

In order to use the p≡p JSON Server Adapter, you need to build and run it. Currently, Linux and OSX/macOS are supported, Windows is about to follow.

### Building on Linux

The p≡p JSON Server Adapter can be build on Debian and Ubuntu. Other flavors may work, but are not officially supported.

#### System requirements

* Debian 9 (or higher) or Ubuntu 16.04 (or higher)

#### Dependencies
* g++ 4.8 or 4.9
* GNU make
* libboost-filesystem-dev
* [p≡p Engine](https://letsencrypt.pep.foundation/trac/wiki/Basic%20Concepts%20of%20p%E2%89%A1p%20engine) (which needs gpgme-thread,
  a patched libetpan, libboost-system-dev)

#### Build steps

1. Build [p≡p Engine](https://letsencrypt.pep.foundation/trac/wiki/Basic%20Concepts%20of%20p%E2%89%A1p%20engine)
2. Build [libevent](http://libevent.org/) (a user-install in $HOME/local/ is fine)
3. Edit the library and include paths server/Makefile so p≡p & libevent will be found
4. Run "make" in the server/ path

```
cd server
make
```

### Building on macOS

The p≡p JSON Server Adapter can be build on OS X or macOS with MacPorts.

#### System requirements

* OS X or macOS

#### Preconditions

For compiling the p≡p JSON Server Adapter and its dependencies, make sure you have the LANG variable set.

```
export LANG=en_US.UTF-8
```

#### Dependencies

The following dependencies need to be installed in order to be able to build the p≡p JSON Server Adapter.

##### MacPorts
[Install MacPorts](https://www.macports.org/install.php) for your version of OS X/macOS.

If MacPorts is already installed on your machine, but was installed by a different user, make sure your `PATH` variable is set as follows in `~/.profile`:

```
export PATH="/opt/local/bin:/opt/local/sbin:$PATH"
```

Install dependencies packaged with MacPorts as follows.

```
sudo port install openssl boost ossp-uuid
```

##### libevent

Install [libevent](http://libevent.org/).

```
cd libevent-2.0.22-stable
export LDFLAGS=-L/opt/local
export CFLAGS=-I/opt/local/include

./configure --prefix "$HOME"

make
make install
```

#### Build steps

1. Install dependencies 
2. Run "make" in the server/ path

```
cd server
make
```

### Running the pEp JSON Adapter

1. Run ./pep-json-server.  This creates a file that is readable only by the
   current user (/tmp/pEp-json-token-${USER}) and contains the address and
   port the JSON adapter is listening on, normally 127.0.0.1:4223 and a
   "security-token" that must be given in each function call to authenticate
   you as the valid user. 

   ```
   ./pep-json-server
   ```  

2. Visit that address (normally http://127.0.0.1:4223/) in your
   javascript-enabled web browser to see the test JavaScript client.
3. Call any function ("version()" or "get_gpg_path()" should work just
   fine) with the correct security token.

## Using the p≡p JSON Adapter

In the following section, you'll find background information on how to use
the adapter and its functions.

### Preliminary notes

* When using the p≡p engine, a session is needed to which any adapter can connect.
In case of the p≡p JSON Server Adapter, it creates this session automatically and
attaches to / detaches from it automatically. Therefore, the client does not need
to take care of the session management. However, it might be necessary to set up a
[HTTP persistent connection](https://en.wikipedia.org/wiki/HTTP_persistent_connection).

### API Principles

All C data types are mapped the same way, so some day the JSON wrapper can be generated from the p≡p Engine header files (or the JSON wrapper and the p≡p engine header are both generated from a common interface description file)

| C type | JSON mapping |
|--|--|
| `bool` | JSON boolean |
| `int` | JSON decimal number | 
| `char*` (representing a UTF-8-encoded NULL-terminated string | JSON string |
| `char*` (representing a binary string | base64-encoded JSON string |
| `enum` | string with the enumerator constant (e.g. `PEP_KEY_NOT_FOUND` ) |
| `struct` | JSON object |
| linked lists (e.g. `bloblist_t`, `stringlist_t`, `identity_list` etc.) | JSON array of their member data type (without the `next` pointer) |

### API Reference

An complete overview with all functions that are callable from the client can be found in the [API Reference](pEp JSON Server Adapter/API Reference).

## Extending / customizing 

If you want to extend or customize the p≡p JSON Adapter, there are several
rules and defitions to take into account.

### General

* At the moment only functions with a non-void return type are supported. 
  It is possible to extend the FunctionMap to support also void-returning
  functions if desired, but it would require more template specializations
  in function_map.hh etc.  The alternative is a helper function that calls
  the void function and just returns a dummy value.

### Definitions

* The 'FunctionMap function' in mt-server.cc defines which functions are
  callable via the JSON-RPC interface.  The existing entries show the syntax
  of that map.

* The current implementation supports input and output parameters, no
  "inout".

* For each type there must exist specializations of the template classes
  "In" (for input parameters) and "Out" (for output parameters).
  The linker will tell you, which specializations are needed.

* The specializations for "generic types" are in function_map.cc

* The specializations for "p≡p-specific types" are in pep-types.cc



## TODOs

The following issues are planned but not yet implemented.

* Windows build:
    * implement get_token_filename() for MS Windows (security-token.cc line 43)
    * do the Windows-specific stuff to build the software on Windows

* Add unit tests

* Fix the bugs that are found by the Unit tests, if any.

* Generate all the tedious boiler plate code
    * the content of pep-types.cc
    * perhaps the FunctionMap 'function' in mt-server.cc
    * perhaps the JavaScript side of the HTML test page to ensure to be
      consistent with the server side in pep-types.cc

* Adapt the "p≡p Transport API", when it is final. (either manually or by
  code generator, if ready)
