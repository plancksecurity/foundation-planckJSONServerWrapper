#ifndef JSON_SERVER_ADAPTER_DAEMONIZE_HH
#define JSON_SERVER_ADAPTER_DAEMONIZE_HH

#include <cstdint>

// cstdint uintptr_t is optional
#ifndef uintptr_t
#define ptrdiff_t uintptr_t
#endif

// fork(), go into background, close all ttys etc...
// system-specific! (POSIX, Windows, ...?)
void daemonize(const bool daemonize, const uintptr_t winsrv);
void daemonize_commit(int retval);

#endif
