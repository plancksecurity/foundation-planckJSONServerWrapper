# p≡p JSON Server Adapter

## Introduction
The p≡p JSON Server Adapter provides a REST-like jQuery-compatible API to
connect with the p≡p engine.  It is language-independent and can be used by
any client.

## Requirements
In order to use the p≡p JSON Server Adapter, you need to build and run it.
Currently, Linux (Debian 9, Ubuntu 16.04) and MacOS (10.11, 10.12) are
supported, Windows is about to follow.  Newer versions should also work
(file a bug report if not) but are not in our main focus, yet.

## Dependencies
* C++ compiler: tested with g++ 4.8 and 4.9, and clang++ 2.8. Newer versions should work, too.
* GNU make
* libboost-thread-dev (tested with 1.58, 1.62 and 1.70)
* libboost-program-options-dev  (tested with 1.58, 1.62 and 1.70)
* libboost-filesystem-dev (tested with 1.58, 1.62 and 1.70)
* libevent-dev 2.0.21 or 2.0.22 (or build from source, see below)
* [p≡p Engine](https://pep.foundation/dev/repos/pEpEngine/)
  (which needs gpgme-thread, a patched libetpan, libboost-system-dev)
* [libpEpAdapter](https://pep.foundation/dev/repos/libpEpAdapter/)
* OSSP libuuid

## Building/Installing (Linux and macOS)
### Install the dependencies
Debian 9:

~~~~~
apt install -y build-essential libboost1.62-dev libboost-system1.62-dev \
    libboost-filesystem1.62-dev libboost-program-options1.62-dev \
    libboost-thread1.62-dev libgpgme-dev uuid-dev googletest \
    libevent-dev libevhtp-dev
~~~~~

macOS 10.12:

Use homebrew or macports to install the required libraries.

For more explicit instructions on how to do this with macports, see the
section below.

Build and install the pEp Engine.  Instructions can be found here:
[https://cacert.pep.foundation/dev/repos/pEpEngine/file/ef23982e4744/README.md](https://cacert.pep.foundation/dev/repos/pEpEngine/file/ef23982e4744/README.md).

### Build and install libevent

It is recommended to use the libevent from your system's repository, if it contains the right version.

~~~~~
mkdir ~/code/json-ad
hg clone https://cacert.pep.foundation/dev/repos/pEpJSONServerAdapter/ ~/code/json-ad
cd ~/code/json-ad/libevent-2.0.22-stable
./configure --prefix="$HOME/code/json-ad/libevent-2.0.22-stable/build/" --disable-openssl
make
make install
~~~~~

### Build and install the JSON server
~~~~~
cd ~/code/json-ad/server
~~~~~

Edit the build configuration to your needs in `./Makefile.conf`, or create a
`./local.conf` that sets any of the make variables documented in
`./Makefile.conf`.

If a dependency is not found in your system's default include or library
paths, you will have to specify the according paths in a make variable. 
Typically, this has to be done at least for the pEp Engine, libetpan and
libevent.

Below are two sample `./local.conf` files, for orientation.

macOS 10.12:

~~~~~
PREFIX=$(HOME)/code/json-ad/build
HTML_DIRECTORY=$(PREFIX)/share/pEp/json-adapter/html
GTEST_DIR=$(HOME)/code/gtest/googletest

BOOST_INC=-I$(HOME)/Cellar/boost/1.65.1/include
BOOST_LIB=-L$(HOME)/Cellar/boost/1.65.1/lib

ENGINE_INC=-I$(HOME)/code/engine/build/include
ENGINE_LIB=-L$(HOME)/code/engine/build/lib

ETPAN_INC=-I$(HOME)/code/libetpan/build/include
ETPAN_LIB=-L$(HOME)/code/libetpan/build/lib

EVENT_INC=-I$(HOME)/code/json-ad/libevent-2.0.22-stable/build/include
EVENT_LIB=-L$(HOME)/code/json-ad/libevent-2.0.22-stable/build/lib

GPGME_INC=-I$(HOME)/Cellar/gpgme/1.9.0_1/include
GPGME_LIB=-L$(HOME)/Cellar/gpgme/1.9.0_1/lib

UUID_INC=-I$(HOME)/Cellar/ossp-uuid/1.6.2_2/include
UUID_LIB=-L$(HOME)/Cellar/ossp-uuid/1.6.2_2/lib
~~~~~

Debian 9:

~~~~~
PREFIX=$(HOME)/code/json-ad/build
HTML_DIRECTORY=$(PREFIX)/share/pEp/json-adapter/html
GTEST_DIR=/usr/src/googletest/googletest/

ENGINE_INC=-I$(HOME)/code/engine/build/include
ENGINE_LIB=-L$(HOME)/code/engine/build/lib

ETPAN_INC=-I$(HOME)/code/libetpan/build/include
ETPAN_LIB=-L$(HOME)/code/libetpan/build/lib

EVENT_INC=-I$(HOME)/code/json-ad/libevent-2.0.22-stable/build/include
EVENT_LIB=-L$(HOME)/code/json-ad/libevent-2.0.22-stable/build/lib
~~~~~

Now, build and install the server:

~~~~~
make all
make install
~~~~~

If you only want to build the JsonAdapter library, run `make lib` and you'll get
a libjson-adapter.a


With `make test` you can execute the server's tests.


### Macports
[Install MacPorts](https://www.macports.org/install.php) for your version of macOS.

If MacPorts is already installed on your machine, but was installed by a
different user, make sure your `PATH` variable is set as follows in
`~/.profile`:

```
export PATH="/opt/local/bin:/opt/local/sbin:$PATH"
```

Install dependencies packaged with MacPorts as follows.

```
sudo port install gpgme boost ossp-uuid
```

## Building/Installing (Windows)
Clone the repository from https://pep.foundation/dev/repos/pEpJSONServerAdapter and add the
following projects to your MS Visual Studio solution:
pEpJSONServerAdapter\build-windows\pEpJSONServerAdapter\pEpJSONServerAdapter.vcxproj
pEpJSONServerAdapter\build-windows\pEpJSONServerAdapter\pEpJSONServerAdapterLibrary.vcxproj
pEpJSONServerAdapter\build-windows\libevent\libevent.vcxproj

In order to build, the MS VS solution also needs to build the following dependent projects:
- pEpEngine (note that this requires further dependent projects)
- libpEpAdapter
- libevent

The resulting executable is called pEpJSONServerAdapter.exe and will be placed into
the `Debug` or `Release` directory of the solution.

## Running the pEp JSON Adapter
You can use `make run` to start the server.

1. Run ./pep-json-server.  This creates a file that is readable only by the
   current user (~/.pEp/json-token-${USER}) and contains the address and
   port the JSON adapter is listening on, normally 127.0.0.1:4223 and a
   "security-token" that must be given in each function call to authenticate
   you as the valid user.

   ```
   ./pep-json-server
   ```

2. Visit that address (normally http://127.0.0.1:4223/) in your
   JavaScript-enabled web browser to see the "JavaScript test client".
3. Call any function ("version()" or "get_gpg_path()" should work just
   fine) with the correct security token.

## Using the p≡p JSON Adapter

In the following section, you'll find background information on how to use
the adapter and its functions.

### Server startup and shutdown

The JSON Server Adapter can be started on demand.
It checks automatically whether an instance for the same user on the machine
is already running and if yes it ends itself gracefully.

If there is no running server found the newly started server creates the
server token file and forks itself into background (if not prevented via
"-d" commandline switch).


### Session handling

When using the p≡p engine, a session is needed to which any adapter can
connect. The p≡p JSON Server Adapter automatically creates one session per
HTTP client connection (and also closes that session automatically when the
client connections is closed). Therefore, the client does not need to take
care of the session management. However, the client has to set up a [HTTP
persistent
connection](https://en.wikipedia.org/wiki/HTTP_persistent_connection).

### API Principles

All C data types are mapped the same way, so some day the JSON wrapper can
be generated from the p≡p Engine header files (or the JSON wrapper and the
p≡p engine header are both generated from a common interface description
file).

| C type | JSON mapping |
|--|--|
| `bool` | JSON boolean |
| `int` | JSON decimal number |
| `size_t` | JSON decimal number |
| `char*` (representing a UTF-8-encoded NULL-terminated string | JSON string |
| `char*` (representing a binary string | base64-encoded JSON string |
| `enum` | either JSON decimal number or JSON object containing one decimal number as member |
| `struct` | JSON object |
| linked lists (e.g. `bloblist_t`, `stringlist_t`, `identity_list` etc.) | JSON array of their member data type (without the `next` pointer) |

The parameter type PEP_SESSION is handled automatically by the JSON Server
Adapter and the PEP_SESSION parameter is omitted from the JSON API.

#### enum types

Enum types are represented as JSON objects with one member, whose name is
derived from the enum type name, holding the numeric value of the enum.

Some enum types are still represented directly as JSON decimal number. It
shall be changed in a future version of the JSON Adapter.

#### String types

The JSON Server Adapter does automatic memory management for string
parameters. The current p≡p Engine's API distinguish between `const char*`
parameters and `char*` parameters.  `const char*` normally means: the
"ownership" of the string remains at the caller, so the JSON Adapter frees
the string automatically after the call.  `char*` normally means: the
"ownership" of the string goes to the Engine, so the JSON Adapter does _not_
free string.

If there are functions that have a different semantics the behavior of the
JSON wrapper has to be changed.

#### Parameter (value) restrictions

Some API functions have restrictions on their parameter values.  The JSON
Adapter does not know these restrictions (because it does not know the
semantics of the wrapped functions at all).  So it is the client's
responsibility to fulfill these parameter restrictions!  Especially when
there are restrictions that are checked with assert() within the p≡p Engine,
it is impossible for the JSON Adapter to catch failed assertions - the
Engine and the Adapter process will be terminated immediatetely when the
Engine is compiled in debug mode (= with enabled assert() checking).

Currently there are no range checks for numerical parameter types (e.g. a
JSON decimal number can hold a bigger value than the `int` parameter type of
a certain C function).

### API Reference

An complete overview with all functions that are callable from the client
can be found in the [API Reference](pEp JSON Server Adapter/API Reference).

That API reference is a generated file that shows the current API briefly.
There is also a (currently manually written) file that holts a copy of the
documentation from the Engine's header files: [API reference detail.md]

Most of the callable functions are functions from the C API of the p≡p
Engine.  They are described in detail, incl.  pre- and post-conditions in
the appropriate C header files of the Engine.


### Authentication

The JSON Server Adapter and the client have to authenticate to each other.
"Authentication" in this case means "run with the same user rights". This is
done by proving that each communication partner is able to read a certain
file that has user-only read permissions.

0. There is a common (between client & server) algorithm to create the path
   and filename of the "server token file", for a given user name.
   The token file and its directory MUST be owned by the user and MUST be
   readable and writable only by the user, nobody else.  Client and server
   check for the right ownership and access rights of the token file and its
   directory. (TODO: What shall be done if that check fails?)

1. The server creates a "server token file" containing a "server token" (a
   random-generated string of printable ASCII characters) and the IP address
   and port where the server listens on.  This file can only be read by
   client programs that run with the same user rights.

2. The client checks the path, reads the "server token" from the file and
   authenticates itself to the server in each JSON RPC call with that "server
   token".


### Callbacks / Reverse connection

p≡p applications must register callback handlers at the Engine. At the moment
there are these callbacks:

* `PEP_STATUS messageToSend(message* msg)`
* `PEP_STATUS notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal signal)`

The JSON adapter register its own functions at the Engine which propagate these
events to all connected clients.

The event propagation to the clients are also done via JSON RPC calls. Here
the JSON Adapter acts as "client" which connects to a host:port that was told
to by the Client to the JSON Adapter via `registerEventListener()` call, where
Client also sends a security token to the JSON Adapter for the reverse JSON RPC
connection.

Note: It is planned to change the way how events are sent to the Client, when
the Client is unable to open listen sockets (it is rumored that future versions
of Mozilla Thunderbird's plugin API will no longer allow this).

Idea 1: Use long polling: The Client calls a function that blocks until an event
from the Engine arrives. This approach requires only a few changes in the JSON Adapter.

Idea 2: Use WebSockets: In fact this is also a type of "long polling" and an open
TCP connection, opened by the Client. But it requires additional code in the JSON
Adapter for the WebSockets protocol and a mechanism how to transfer the underlaying
TCP socket and buffer from the libevent library to the WebSockets implementation.
See: https://pep.foundation/jira/browse/JSON-128


## Extending / customizing

If you want to extend or customize the p≡p JSON Adapter, there are several
rules and definitions to take into account.

### API Functions

* The `FunctionMap function` in `ev_server.cc` defines which functions
  are callable via the JSON-RPC interface.  The existing entries show the
  syntax of that map.
  Non-static member functions can be called, too. Thanks to `std::function<>`
  a member function `Foo::func(Params...)` is handled like a free-standing
  function `func(Foo* f, Params...)`.

* For each type there must exist specializations of the template classes
  "In" (for input parameters) and "Out" (for output parameters).
  The linker will tell you, which specializations are needed.

* The specializations for "generic types" are in `function_map.cc`.

* The specializations for "p≡p-specific types" are in `pep-types.cc`.


#### Parameter directions (In, Out, InOut)

The p≡p JSON Server Adapter supports Input, Output and two ways of "In/Out"
parameters.  You have to annotate the direction in the FunctionMap with
`In<>` for input, `Out<>` for output and `InOut<>` or `InOutP<>` for in/out
parameters.  These wrapper classes have an optional second template
parameter (parameter type flag) that is explained below.

Return values are always "output" parameters, so they don't have to be
wrapped with `Out<>`, but this wrapper is necessary when you need
non-default wrapper semantics, see below.


Input parameters of fundamental or simple struct types are
usually by-value parameters. Complex structs (or structs that are only
forward-declared in the public API) are usually pointer
parameters. Both ways are supported. You have to specialize `In<T>` or
`In<T*>`, depending how your type is used.

Output parameters of fundamental or simple struct types `T` are usually
declared as a paremeter of type `T*`. The p≡p JSON Server Adapter manages
the memory allocated by the called C function automatically and calls the
appropriate de-allocating function after use.

Calling a function with output parameters requires a dummy value (`null` or
empty string is fine) at the JSON side for each output parameter to keep the
number of parameters at the JSON side the same with the C side.

For In/Out parameters there exist two calling conventions for
call-by-pointer types:

1. caller allocates object and fills with input values, callee can only *change members*.
The C type of the parameter is usually `struct T*`. Use the wrapper `InOut<T*>`
for these parameters.

2. caller allocates object and fills with input values, callee might
change/reallocate the *whole object*. The C type of the parameter is
`struct T**`. Use the wrapper `InOutP<T*>` in these cases.

`InOutP<T>` is also the right wrapper for in/out parameters of fundamental or
enum types due to the additional indirection in the C function call
signature.


#### Parameter type flags

The wrapper classes might be instantiated with special "parameter type
flags". If no flag is given the `DefaultFlag` is used with means the
semantics described already above.

At the moment there exist two parameter type flags which are interpreted as
bitfield, so they can be combined:

* NoInput : This denotes a parameter at the C side that shall *not be exposed*
  at the JSON side. So the value cannot be specified by the client, it is
  provided by the JSON Server Adapter internally (e.g. for PEP_SESSION)

* DontOwn : Used for pointer types who don't "own" the referred ressource,
  so it is not released automatically by the JSON Server Adapter after the
  call.

More flags will be added when different semantics will be needed.


#### Automatic parameter value generation

For some parameters or parameter combinations the JSON Server Adapter is
able to generate the values automatically either from the environment or
from other parameters.

These automatic parameter value generators are supported at the moment:

##### In<c_string> and InLength

For functions that have a string parameter of type `const char*` followed by
a `size_t` that specifies the length of the string, the JSON Adapter can
calculate the value of that length parameter automatically, because in the
JSON API the lengths of strings are always known.

Moreover, the "length" that has to be given here means the length of the
string seen by the C API side after processing of all JSON escaping
mechanisms as raw UTF-8 NFC string, so it might be difficult to calculate
that value at client side.

The "magic" is done inside the In<c_string> constructor that stores the string
length in its "Context", and the InLength<> constructore retrieves the value
from its "Context".

Example:
```
// C function declaration:
char* tohex(const char* input, size_t length);

// API definition:
// with implicit length parameter, with dummy JSON parameter
FP( "tohex", new Func<char*, In<c_string>, InLength<>>( &tohex ))
```

To be compatible with previous API versions the `InLength` parameter still
needs a dummy placeholder in the JSON interface, but its value is no longer
relevant:

```
{"jsonrpc":"2.0", "id":28,
 "method":"tohex", "params":["some string","dummy_parameter"]
}
```

It is possible to specifiy `InLength<ParamFlag::NoInput>` so no
parameter is exposed to the JSON API anymore:

```
FP( "tohex", new Func<char*, In<c_string>, InLength<ParamFlag::NoInput>>( &tohex ))
```

Now the 2nd parameter is omitted:
```
{"jsonrpc":"2.0", "id":28,
 "method":"tohex", "params":["some string"]
}
```

### Embedding in other (desktop) adapters

The JSON Adapter can run as a stand-alone program (called the "mini-adapter") or
as part of another desktop adapter to enhance that adapter with a JSON-RPC interface.

For this the JSON Adapter has to co-operate with the desktop adapter in several ways:

* Startup, configuration and shutdown is managed by the desktop adapter.

* Handshake events and sync messages created by the pEpEngine have to be dispatched
  to *all* connected clients, no matter whether they are JSON clients or "native"
  clients of the desktop adapter. See "messageToSend" and "notifyHandshake" callbacks.

* The sync thread loop has to be managed by the desktop adapter. The libpEpAdapter
  contains an example implementation for that.

* (something else?)


## TODOs

The following issues are planned but not yet implemented.

* More sensible unit tests

* Generate all the tedious boiler plate code
    * the content of pep-types.cc
    * perhaps the FunctionMap 'function' in mt-server.cc
    * perhaps the JavaScript side of the HTML test page to ensure to be
      consistent with the server side in pep-types.cc

* Adapt the "p≡p Transport API", when it is final. (either manually or by
  code generator, if ready)


## Appendix A: Attack scenarios on the authentication

Let's discuss different attack / threat scenarios. I don't know which are
realistic or possible, yet.

### General ideas / improvements

Currently the JSON Server Adapter writes its server token file in a
directory that is only readable & writable by the user itself.

The server token file is written in $HOME/.pEp/json-token on
UNIX/Linux/MacOS and %LOCALAPPDATA%/pEp/json-token on MS Windows.

The JSON Server Adapter also checks whether .pEp has 0700 access rights
on unixoid systems.


### Attacker with the same user rights

If the attacker is able to run his malicious code with the same user
rights as the JSON Server Adapter and his legitimate client, it is (and
always will be) *impossible* to prevent this attack. Such an attacker also
can just start a legitimate client that is under his control.

The same applies to an attacker who gains root / admin access rights.

### Fake Server with different user rights

```
 ,----------.      ,--------.
 | Attacker | <==> | Client |
 `----------'      `--------'
```

If no real JSON Adapter runs an attacker can create a fake server that
pretends to be a legitimate JSON Adapter. It creates its own server token
file, with different and conspicuous access rights, but a limited
JavaScript client might be unable to detect the file permissions.

This fake server cannot access the private key of the user but it might
get sensitive plaintext data the client wants to encrypt. The fake server
cannot sign the encrypted data so the fake would be conspicuous, too. But
that would be too late, because the sensitive plaintext data could
already be leaked by the fake server.

This attack needs a user's home directory that is writable by someone else
(to create a ~/.pEp/ directory) or a foreign-writable ~/.pEp/ directory.

The pEpEngine creates a ~/.pEp/ directory (if not yet exists) and sets the
permissions to 0700 explicitly.


### Man-in-the-middle with different user rights

```
 ,---------------------.      ,----------.      ,--------.
 | JSON Server Adapter | <==> | Attacker | <==> | Client |
 `---------------------'      `----------'      `--------'
```

* The attacker cannot read "client token file" nor "server token file".
* The server cannot check "who" connects to it, until the client
  authenticates itself, which might be relayed by the attacker from the
  original client.
* The attacker has to convince the client that it is a legitimate server. It
  has to create a fake "server token file" to divert the client to the
  attacker's port. But that fake file cannot contain the right server token
  because the attacker does not know it.
  * if the server started before the attacker the "server token file"'s
    access rights should prevent this (no write access for the attacker, no
    "delete" right in common TEMP dir (sticky bit on the directory)
  * if the attacker starts before the server it can write a fake toke file.
    The server could detect it but is unable to notice the legitimate
    client. The client could detect it when it can check the file access
    rights.
    There might be race conditons...

* Is it possible for the attacker to let the client send the right server
  token to him, at least temporarily (e.g. within a race condition)?
  * As long as the server runs, the attacker cannot bind to the same address
    & port. Finding and binding of the port is done by the server before the
    server token file is created and filled.
  * When the server that created the "server token file" dies, its port
    becomes available for the attacker, but the server token is no longer
    valid and no longer useful for the attacker.
* there _might_ be a very _small_ chance for a race condition:
  1. The attacker opens a connection to the running server but does not
    use it. To find the server it cannot read the server configuration
    file, but it can ask the OS for ports that are open in "listen" mode.
    Normally the JSON Adapter listens on 4223 or some port numbers above
    that. That means: guessing the server's address is quite easy.
  2. when the server shuts down, the attacker immediately binds itself to
    that port. If a client connects just in this moment it sends the server
    token to the attacker, not to the server. But the attacker can use that
    token now to the server via the already opened TCP connection.
  3. To prevent this the server should call shutdown(2) on its listening
    socket to block any new client connection, but still block the port.
    (is that the case on all platforms?) Than close(2) all TCP connections
    to the clients (if any) and than also delete the server token file.
    Finally call close(2) on the listening socket.
