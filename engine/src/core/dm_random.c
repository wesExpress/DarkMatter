#include "dm_random.h"
#include "mt19937/mt19937.h"
#include "mt19937/mt19937_64.h"
#include "core/dm_logger.h"
#include <limits.h>

mt19937 context;
mt19937_64 context_64;

bool dm_random_init(uint32_t seed)
{
    init_genrand(&context, seed);
    init_genrand64(&context_64, seed);
    
    return true;
}

void dm_random_shutdown()
{
    
}

int dm_random_int()
{
    return dm_random_uint32() - INT_MAX;
}

int dm_random_int_range(int start, int end)
{
    int old_range = UINT_MAX;
    int new_range = end - start;
    
    return ((dm_random_int() + INT_MAX) * new_range) / old_range + start;
}

uint32_t dm_random_uint32()
{
    return genrand_int32(&context);
}

uint32_t dm_random_uint32_range(uint32_t start, uint32_t end)
{
    uint32_t old_range = UINT_MAX;
    uint32_t new_range = end - start;
    
    return (dm_random_uint32() * new_range) / old_range + start;
}

uint64_t dm_random_uint64()
{
    return genrand64_uint64(&context_64);
}

uint64_t dm_random_uint64_range(uint64_t start, uint64_t end)
{
    uint64_t old_range = ULLONG_MAX;
    uint64_t new_range = end - start;
    
    return (dm_random_uint64() - new_range) / old_range + start;
}

float dm_random_float()
{
    return genrand_float32_full(&context);
}

float dm_random_float_range(float start, float end)
{
    DM_LOG_ERROR("This is not implemented yet!");
    return 0.0f;
}

double dm_random_double()
{
    return genrand64_real1(&context_64);
}

double dm_random_double_range(double start, double end)
{
    DM_LOG_ERROR("This is not implemented yet!");
    return 0.0f;
}
