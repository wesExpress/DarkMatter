#ifndef __DM_EVENT_H__
#define __DM_EVENT_H__

#include <stdbool.h>

typedef enum dm_event_type
{
	DM_WINDOW_CLOSE_EVENT, DM_WINDOW_RESIZE_EVENT,
	DM_MOUSEBUTTON_DOWN_EVENT, DM_MOUSEBUTTON_UP_EVENT, DM_MOUSE_MOVED_EVENT, DM_MOUSE_SCROLLED_EVENT,
	DM_KEY_DOWN_EVENT, DM_KEY_UP_EVENT, DM_KEY_TYPE_EVENT
} dm_event_type;

typedef bool (*dm_event_callback)(dm_event_type type, void* data);

typedef struct dm_event
{
	dm_event_type type;
	dm_event_callback callback;
	void* data;
} dm_event;

bool dm_event_dispatch(dm_event event);

void dm_event_set_callback(dm_event_callback callback);

#endif