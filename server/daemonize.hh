#ifndef JSON_SERVER_ADAPTER_DAEMONIZE_HH
#define JSON_SERVER_ADAPTER_DAEMONIZE_HH

#include <functional>

// fork(), go into background, close all ttys etc...
// child_fn is called in the child process, before it detaches from the tty.
// system-specific! (POSIX, Windows, ...?)
void daemonize( std::function<void()> child_fn = std::function<void()>() );

#endif
