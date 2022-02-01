#include "dm_event.h"
#include "dm_assert.h"
#include <stdlib.h>

static dm_event_callback app_callback = NULL;

bool dm_event_dispatch(dm_event e)
{
	DM_ASSERT_MSG(app_callback, "Event callback has not been set!");

	//if (e.callback) e.callback(e.data);
	app_callback(e.type, e.data);
	return false;
}

void dm_event_set_callback(dm_event_callback callback)
{
	app_callback = callback;
}