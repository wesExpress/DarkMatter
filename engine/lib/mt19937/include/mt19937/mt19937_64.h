#ifndef __MT19937_64_H__
#define __MT19937_64_H__

/*
see:
http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/VERSIONS/C-LANG/mt19937-64.c

https://github.com/mattgallagher/CwlUtils/blob/master/Sources/ReferenceRandomGenerators/mt19937-64.c

*/

#include <stdint.h>

typedef struct mt19937_64
{
	uint64_t mt[312];
	size_t mti;
} mt19937_64;

/* initializes mt[NN] with a seed */
void init_genrand64(mt19937_64* context, uint64_t seed);

/* generates a random number on [0, 2^63-1]-interval */
uint64_t genrand64_uint63(mt19937_64* context);

/* generates a random number on [0, 2^64-1]-interval */
uint64_t genrand64_uint64(mt19937_64* context);

/* generates a random number on [0,1]-real-interval */
double genrand64_real1(mt19937_64* context);

/* generates a random number on [0,1)-real-interval */
double genrand64_real2(mt19937_64* context);

/* generates a random number on (0,1)-real-interval */
double genrand64_real3(mt19937_64* context);

#endif