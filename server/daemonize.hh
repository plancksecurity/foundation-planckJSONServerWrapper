#ifndef JSON_SERVER_ADAPTER_DAEMONIZE_HH
#define JSON_SERVER_ADAPTER_DAEMONIZE_HH

// fork(), go into background, close all ttys etc...
// system-specific! (POSIX, Windows, ...?)
void daemonize();

#endif
