#ifndef __DM_APP_CONFIG_H__
#define __DM_APP_CONFIG_H__

#include "dm_engine_types.h"

typedef struct dm_application
{
	dm_engine_config engine_config;
	void* state;
} dm_application;

#endif