#include "prefix-config.hh"

// #define HTML_DIR via -D commandline / Makefile option

#ifndef HTML_DIR
#define HTML_DIR "../html"
#endif

#define STR(x) #x

const char* const html_directory = STR(HTML_DIR);
