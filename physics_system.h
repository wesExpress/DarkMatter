#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "dm.h"

bool physics_system_init(dm_context* context);
void physics_system_shutdown(void* s, void* c);
bool physics_system_run(void* s, void* c);

#endif //PHYSICS_SYSTEM_H
