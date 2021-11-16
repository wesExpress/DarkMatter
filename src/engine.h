#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "defines.h"

typedef struct engine_data
{
    void* platform_data;
} engine_data;

bool engine_create();
void engine_shutdown();

bool engine_run();

#endif