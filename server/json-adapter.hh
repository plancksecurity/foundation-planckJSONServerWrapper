#ifndef MT_SERVER_HH
#define MT_SERVER_HH

#include <pEp/pEpEngine.h>
#include "registry.hh"

typedef Registry<PEP_SESSION, PEP_SESSION(*)(), void(*)(PEP_SESSION)>  SessionRegistry;

extern SessionRegistry session_registry;

#endif // MT_SERVER_HH