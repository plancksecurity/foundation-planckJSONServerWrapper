# p≡p JSON Adapter

## Introduction

The JSON Adapter provides a REST-like jQuery-compatible API to connect with
the p≡p engine.  It is language-independent and can be used by any client.
[...]

## Getting started

In order to use the p≡p JSON Adapter, you need to build and run it. [...]

### Build process

The p≡p JSON adapter can be build on any Debian based OS. ??? [...]

#### System requirements

* Debian version ...?
* ...

#### Dependencies
* g++ 4.8 or 4.9
* GNU make
* libboost-filesystem-dev
* [p≡p Engine](link/to/pEp/Engine/documentation) (which needs gpgme-thread,
  a patched libetpan, libboost-system-dev)

#### Build steps

1. Build [p≡p Engine](link/to/pEp/Engine/build/instructions)
2. Build [libevent](link/to/libevent-2.0.22-stable/README] (a user-install in $HOME/local/ is fine)
3. Edit the library and include paths server/Makefile so p≡p & libevent will be found
4. Run "make" in the server/ path

### Running the pEp JSON Adapter

1. Run ./pep-json-server.  This creates a file that is readable only by the
   current user (/tmp/pEp-json-token-${USER}) and contains the address and
   port the JSON adapter is listening on, normally 127.0.0.1:4223 and a
   "security-token" that must be given in each function call to authenticate
   you as the valid user.
2. Visit that address (normally http://127.0.0.1:4223/) in your
   javascript-enabled web browser to see the test JavaScript client.
3. Call some functions ("version()" or "get_gpg_path()" should work just
   fine) with the correct security token.

## Using the p≡p JSON Adapter

In the following section, you'll find background information on how to use
the adapter and its functions.

### General notes

[in case there are general notes as to how the adapter should be used]

### Functions

#### version()

Returns the version number of the JSON adapter.

#### get_gpg_path()

Returns the GPG path ...

[...]



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

* Windows build: (JSON-23)
   * implement get_token_filename() for MS Windows (security-token.cc line 43
   * do the windows-specific stuff to build the software on Windows

* Add unit tests (I'd suggest GoogleTest/gtest? Any complaints?)

* Fix the bugs that are found by the Unit tests, if any.

* Let's generate all the tedious boiler plate code
    * the content of pep-types.cc
    * perhaps the FunctionMap 'function' in mt-server.cc
    * perhaps the JavaScript side of the HTML test page to ensure to be
      consistent with the server side in pep-types.cc

* Adapt the "p≡p Transport API", when it is final.  (either manually or by
  code generator, if ready)
