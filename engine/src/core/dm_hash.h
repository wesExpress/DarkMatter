/* date = March 25th 2022 3:15 pm */

#ifndef __DM_HASH_H__
#define __DM_HASH_H__

#include <stdint.h>

typedef uint32_t dm_hash;
typedef uint64_t dm_hash64;

dm_hash dm_hash_fnv1a(const char* key);
dm_hash dm_hash_32bit(uint32_t key);
dm_hash64 dm_hash_64bit(uint64_t key);

#endif //DM_HASH_H
