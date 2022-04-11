#ifndef __DM_MODEL_H__
#define __DM_MODEL_H__

#include "core/dm_defines.h"
#include <stdbool.h>

void dm_model_loader_init();
void dm_model_loader_shutdown();

DM_API bool dm_load_model(const char* path, bool normalize);

#endif //DM_MODEL_H
