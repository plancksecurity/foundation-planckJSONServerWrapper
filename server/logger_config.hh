#ifndef LOGGER_CONFIG_HH
#define LOGGER_CONFIG_HH

// maximum length of a log message. longer log messages will be clipped
#define LOGGER_MAX_LOG_MESSAGE_LENGTH (8192)

// maximum length of a log line. longer lines will be wrapped/folded
#define LOGGER_MAX_LINE_LENGTH (1000)

// enable logging to syslog
#ifndef _WIN32
#define LOGGER_ENABLE_SYSLOG (1)
#endif

// use ASCII-only characters to tag log lines
// #define LOGGER_USE_ASCII_TAGS (1)


#endif // LOGGER_CONFIG_HH
