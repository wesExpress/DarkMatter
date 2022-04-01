#ifndef __MT19937_H__
#define __MT19937_H__

#include <stdint.h>

typedef struct mt19937
{
	uint32_t mt[624];
	uint32_t mti;
} mt19937;

/* initializes mt[N] with a seed */
void init_genrand(mt19937* context, uint32_t seed);

/* generates a random number on [0,0xffffffff]-interval */
uint32_t genrand_int32(mt19937* context);

/* generates a random number on [0,0x7fffffff]-interval */
uint32_t genrand_int31(mt19937* context);

/* generates a random number on [0,1]-real-interval */
float genrand_float32_full(mt19937* context);

/* generates a random number on [0,1)-real-interval */
float genrand_float32_notone(mt19937* context);

#endif