 1. Dependencies
=================

Debian
------
* g++ 4.8 or 4.9
* GNU make
* libboost-filesystem-dev
* p≡p Engine
  (which needs gpgme-thread, a patched libetpan, libboost-system-dev)

It comes with libevent 2.20, that needs GNU autohell to build. :-/
Maybe Debian Jessie's version 2.0 also works well, I never tried.


 2. Build
==========

* build p≡p Engine

* first build libevent, see libevent-2.0.22-stable/README
  (a user-install in $HOME/local/ is fine)

* edit the library and include paths server/Makefile so p≡p & libevent will
  be found

* run "make" in the server/ path


 3. Running
============

* run ./mt-server

* visit http://127.0.0.1:4223/ in your javascript-enabled web browser to see
  the test JavaScript client

* call some functions ("version()" or "get_gpg_path()" should work just fine)


 4. Extending / Customizing
============================

* The 'FunctionMap function' in mt-server.cc defines which functions are
  callable via the JSON-RPC interface. The existing entries show the syntax
  of that map.

* At the moment only functions with a non-void return type ere supported.
  It is possible to extend the FunctionMap to support also void-returning
  functions if desired, but it would require more template specializations
  in function_map.hh etc. The alternative is a helper function that calls
  the void function and just returns a dummy value.

* The current implementation supports input and output parameters, no "inout".

* For each type there must exist specializations of the template classes
  "In" (for input parameters) and "Out" (for output parameters).
  The linker will tell you, which specializations are needed.  ;-)

* The specializations for "generic types" are in function_map.cc

* The specializations for "p≡p-specific types" are in pep-types.cc


 5. TODO
=========

* Add unit tests (I'd suggest GoogleTest/gtest? Any complaints?)

* Fix the bugs that are found by the Unit tests, if any.

* Let's generate all the tedious boiler plate code
    * the content of pep-types.cc
    * perhaps the FunctionMap 'function' in mt-server.cc
    * perhaps the JavaScript side of the HTML test page to ensure to be
      consistent with the server side in pep-types.cc

* Adapt the "p≡p Transport API", when it is final.
  (either manually or by code generator, if ready)
