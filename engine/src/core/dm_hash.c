#include "dm_hash.h"

dm_hash dm_hash_fnv1a(const char* key)
{
    dm_hash64 hash = 14695981039346656037UL;
	for (int i = 0; key[i]; i++)
	{
		hash ^= key[i];
		hash *= 1099511628211;
	}
    
	return hash;
}

dm_hash dm_hash_32bit(uint32_t key)
{
    dm_hash hash = ((key >> 16) ^ key) * 0x119de1f3;
    hash= ((hash >> 16) ^ hash) * 0x119de1f3;
    hash= (hash >> 16) ^ hash;
    return hash;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
dm_hash64 dm_hash_64bit(uint64_t key)
{
	dm_hash64 hash = (key ^ (key >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
	hash = hash ^ (hash >> 31);
    
	return hash;
}