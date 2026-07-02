#include <volk.h>
#include <stdarg.h>
#include <stdio.h>

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_DEBUG_LOG(format, ...) do { \
       printf(format, ##__VA_ARGS__); \
       printf("\n"); \
   } while(false)
#include "vk_mem_alloc.h"
