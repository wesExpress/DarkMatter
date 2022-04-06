#ifndef __DM_RANDOM_H__
#define __DM_RANDOM_H__

#include "core/dm_defines.h"
#include <stdbool.h>
#include <stdint.h>

bool dm_random_init(uint32_t seed);
void dm_random_shutdown();

/*
random int from [-2^31, 2^31]
*/
DM_API int dm_random_int();

/*
random int in specified range
*/
DM_API int dm_random_int_range(int start, int end);

/*
random 32bit int in range [0, 2^32]
*/
DM_API uint32_t dm_random_uint32();

/*
random 32bit int in specified range
*/
DM_API uint32_t dm_random_uint32_range(uint32_t start, uint32_t end);

/*
random 64bit int in range [0, 2^64]
*/
DM_API uint64_t dm_random_uint64();

/*
random 64bit int in specified range
*/
DM_API uint64_t dm_random_uint64_range(uint64_t start, uint64_t end);

/*
random float in range [0,1]
*/
DM_API float dm_random_float();

/*
random float in specified range
*/
DM_API float dm_random_float_range(float start, float end);

/*
random double in range [0,1]
*/
DM_API double dm_random_double();

/*
random double in specified range
*/
DM_API double dm_random_double_range(double start, double end);

#endif //DM_RANDOM_H
