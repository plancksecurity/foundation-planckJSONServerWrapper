#ifndef JSON_SERVER_ADAPTER_DAEMONIZE_HH
#define JSON_SERVER_ADAPTER_DAEMONIZE_HH

#include <cstdint>

#define STATUS_HANDLE "status-handle"

// fork(), go into background, close all ttys etc...
// system-specific! (POSIX, Windows, ...?)
void daemonize(const bool daemonize, const uintptr_t status_handle);
void daemonize_commit(int retval);

#endif
