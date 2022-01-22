#include "dm_string.h"
#include "dm_mem.h"
#include <string.h>

char* dm_strdup(const char* src)
{
    char* dest = (char*)dm_alloc(strlen(src)+1, DM_MEM_STRING);
    strcpy(dest, src);
    return dest;
}

void dm_strdel(char* str)
{
    dm_free(str, strlen(str), DM_MEM_STRING);
}