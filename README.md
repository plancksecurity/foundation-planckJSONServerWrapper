 1. Dependencies
=================

Debian
------
	* g++ 4.8 or 4.9
	* GNU make
	* libboost-filesystem-dev
	* p≡p Engine (which needs gpgme-thread, a patched libetpan, libboost-system-dev)

It comes with libevent 2.20, that needs GNU autohell to build. :-/
Maybe Debian Jessie's version 2.0 also works well, I never tried.


 2. Build
==========

	* build p≡p Engine
	* first build libevent, see libevent-2.0.22-stable/README
	  (a user-install in $HOME/local/ is fine)
	* edit the library and include paths server/Makefile so p≡p & libevent will be found
	* run "make" in the server/ path

 3. Running
============
	* run ./mt-server
	* visit http://127.0.0.1:4223/ in your javascript-enabled web browser to see the test JavaScript client
	* call some functions ("version()" or "get_gpg_path()" should work just fine)


