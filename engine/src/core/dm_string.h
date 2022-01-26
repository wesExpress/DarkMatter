#ifndef __DM_STRING_H__
#define __DM_STRING_H__

#include <stdlib.h>
#include <string.h>

typedef struct dm_string
{
    char* string;
    size_t len;
} dm_string;

char* dm_strdup(const char* src);
void dm_strdel(char* str);

#endif