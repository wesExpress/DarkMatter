#ifndef DM_PLATFORM_MAC_H
#define DM_PLATFORM_MAC_H

#include "dm.h"

#include <pthread/pthread.h>

#import <Cocoa/Cocoa.h>

// window delegate
@interface dm_window_delegate : NSObject <NSWindowDelegate>
{
	dm_platform_data* platform_data;
}

- (instancetype)initWithPlatformData : (dm_platform_data*)platform_data_in;
@end

// for mouse and keyboard input
@interface dm_content_view : NSView <NSTextInputClient>
{
    NSWindow*         window;
	dm_platform_data* platform_data;
}

- (instancetype)initWithWindow: (NSWindow*)window_in AndPlatformData:(dm_platform_data*)platform_data_in;
- (NSRect)getWindowFrame;
@end

@interface dm_app_delegate : NSObject<NSApplicationDelegate>
@end

typedef struct dm_mac_semaphore_t
{
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
    uint32_t v;
} dm_mac_semaphore;

typedef struct dm_mac_task_queue_t
{
    dm_thread_task tasks[DM_MAX_TASK_COUNT];
    uint32_t       count;

    pthread_mutex_t    mutex;
    dm_mac_semaphore has_tasks;
} dm_mac_task_queue;

typedef struct dm_mac_threadpool_t
{
    pthread_mutex_t thread_count_mutex;
    pthread_cond_t  all_threads_idle;
    pthread_cond_t  at_least_one_idle;

    uint32_t num_working_threads;

    dm_mac_task_queue task_queue;

    pthread_t threads[DM_MAX_THREAD_COUNT];
} dm_mac_threadpool;

typedef struct dm_internal_apple_data_t
{
    NSWindow*           window;
    dm_app_delegate*    app_delegate;
    dm_window_delegate* window_delegate;
    dm_content_view*    content_view;
} dm_internal_apple_data;

#endif //DM_PLATFORM_MAC_H
