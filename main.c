#define DM_IMPL
#include "dm.h"

#include "app.h"

typedef enum error_code_t
{
    ERROR_CODE_SUCCESS,
    ERROR_CODE_INIT_FAIL,
    ERROR_CODE_RESOURCE_CREATION_FAIL,
    ERROR_CODE_UPDATE_BEGIN_FAIL,
    ERROR_CODE_UPDATE_END_FAIL,
    ERROR_CODE_RENDER_BEGIN_FAIL,
    ERROR_CODE_RENDER_END_FAIL,
    ERROR_CODE_APP_UPDATE_FAIL,
    ERROR_CODE_APP_RENDER_FAIL,
    ERROR_CODE_UNKNOWN
} error_code;

int main(int argc, char** argv)
{
    error_code e = ERROR_CODE_SUCCESS;
    
    dm_context* context = dm_init(100,100,1000,1000,"test","assets");
    
    if(!context) return ERROR_CODE_INIT_FAIL;
    
    if(!app_init(context)) e = ERROR_CODE_INIT_FAIL;
    
    if(e != ERROR_CODE_SUCCESS)
    {
        dm_shutdown(context);
        getchar();
        return e;
    }
    
    dm_timer t = { 0 };
    uint32_t frame_count = 0;
    // run the app
    bool begin_frame = false;
    dm_timer_start(&t, context);
    while(dm_context_is_running(context))
    {
        if(dm_timer_elapsed(&t, context) > 1)
        {
            DM_LOG_INFO("FPS: %u", frame_count);
            frame_count = 0;
            dm_timer_start(&t, context);
        }
        
        ////////////////////
        dm_start(context);
        ////////////////////
        
        // updating
        if(!dm_update_begin(context)) e = ERROR_CODE_UPDATE_BEGIN_FAIL;
        if(!app_update(context))      e = ERROR_CODE_APP_UPDATE_FAIL;
        if(!dm_update_end(context))   e = ERROR_CODE_UPDATE_END_FAIL;
        
        // rendering
        begin_frame = dm_renderer_begin_frame(context);
        if(!app_render(context))                                e = ERROR_CODE_APP_RENDER_FAIL;
        if(!dm_renderer_end_frame(true, begin_frame, context)) e = ERROR_CODE_RENDER_END_FAIL;
        
        /////////////////////
        dm_end(context);
        /////////////////////
        
        frame_count++;
    }
    
    // cleanup
    app_shutdown(context);
    dm_shutdown(context);
    
    // TODO: remove
    getchar();
    
    return e;
}
