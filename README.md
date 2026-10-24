# DarkMatter

Cross platform and rendering framework that uses a bindless approach to GPU resources.

Supports:

- Vulkan
- Metal

## Resources
DarkMatter uses the following resource types:

- Buffer:  generic container of bytes on the gpu. Further specify vertex, index, or storage
- Texture: right now just 2D supported. Can be sampled or storage
- Render Target: contains a color texture and details on how to load/clear. Specify swapchain for default
- Sampler: used for sampled textures

As DarkMatter uses bindless, it is the user's responsibility to determine how each resource should be used. E.G. if you have a sampled texture, you won't be able to push it to a compute/ray tracing shader to read/write on.

## Per Frame

Behind the scenes, when you call 
```c
dm_render_begin(context)
```
DarkMatter takes care of synchronization, obtains the next swapchain image, and begins the command buffer for you. DarkMatter also then binds the DescriptorHeaps for Vulkan; argument buffers are handled during pipeline binding for Metal.

When calling 
```c
dm_render_end(context)
```
DarkMatter then ends recording of the appropriate command buffers and presents the image to the screen.

Thus, for the user the flow is simply to call begin and end with their render commands between these two calls.

In order to update GPU resources, dm_update_begin and dm_update_end should be called around the update commands for buffers and textures. This is due to restrictions on when certain commands are allowed or not. These should be called before render_begin for the updates to be completed.

## Commands
GPU commands are broken into two kinds: render and compute. Rasterization and raytracing are under the render grouping and allow the user to push resources and draw. Combined with compute pipelines and render targets, modern rendering techniques should be quite simple to implement.

Example instanced rendering of a scene:

```c
dm_resource resources[] = {
    vb_buffer,
    inst_buffer,
};

float clear[] = { 0,0,0,1 };
float depth = 1.f;

dm_render_load_op rt_load      = DM_RENDER_LOAD_OP_CLEAR;
dm_render_store_op rt_store    = DM_RENDER_STORE_OP_STORE;
dm_render_load_op depth_load   = DM_RENDER_LOAD_OP_LOAD;
dm_render_store_op depth_store = DM_RENDER_STORE_OP_DONT_CARE;

dm_render_command_begin_rendering(context, render_target, clear[0],clear[1],clear[2],clear[3],depth, rt_load,rt_store,depth_load,depth_store);
    dm_render_command_bind_pipeline(context, pipeline);
    dm_render_command_set_viewport(context, x,y,w,h,zmin,zmax);
    dm_render_command_set_scissor(context, x,y,w,h);
    dm_render_command_push_resources(context, resources, _countof(resources));
    dm_render_command_bind_index_buffer(context, buffer, ib_offset);
    dm_render_command_draw(context, index_count, index_offset, instance_count, vertex_count);
dm_render_command_end_rendering(context, render_target)
```

For rasterized rendering, we begin by setting the render target (could be custom or the default swapchain), and specifying the clear values and loading and storing options (here we clear the target and store it, we clear the depth buffer and we dont care about storing it). Next we bind the raster pipeline, set our viewport and scissor, bind our index buffer, push our resources and draw. We must also end rendering on our render target.

Pushing resources works differently depending on the render backend: for Vulkan, indexes into a DescriptorHeap are pushed to the shaders in the push_data block; for Metal an argument buffer is filled with the appropriate resources.

## Dependencies
DarkMatter uses GLFW for platform handling. It also comes with DearImgui bundled, specifically with the dear_bindings C bindings. Due to the bindless nature of DarkMatter, we DO NOT USE the render backends provided by DearImgui, only the GLFW backend. As of now, the user is responsible for creating and handling the imgui rendering using the render API. 
