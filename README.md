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

## Commands
Basic commands are abstracted away. Combined with compute pipelines and render targets, modern rendering techniques should be quite simple to implement.

Example instanced rendering of a scene:

'''
dm_resource resources[] = {
    vb_buffer,
    inst_buffer,
};

float clear[] = { 0,0,0,1 };
float depth = 1.f;

dm_render_command_begin_rendering(context, render_target, clear[0],clear[1],clear[2],clear[3],depth, DM_RENDER_LOAD_OP_CLEAR, DM_RENDER_STORE_OP_STORE, DM_RENDER_LOAD_OP_DONT_CARE, DM_RENDER_STORE_OP_STORE);
    dm_render_command_bind_pipeline(context, pipeline);
    dm_render_command_set_viewport(context, 0,0,w,h,zmin,zmax);
    dm_render_command_set_scissor(context, 0,0,w,h);
    dm_render_command_push_resources(context, resources, 2);
    dm_render_command_bind_index_buffer(context, buffer, ib_offset);
    dm_render_command_draw(context, index_count, index_offset, instance_count, vertex_count);
dm_render_command_end_rendering(context, render_target)
'''

For rasterized rendering, we begin by setting the render target (could be custom or the default swapchain), and specifying the clear values and loading and storing options. Next we bind the raster pipeline, set our viewport and scissor, bind our index buffer, push our resources and draw. We must also end rendering on our render target.

Pushing resources works differently depending on the render backend: for Vulkan, indexes into a DescriptorHeap are pushed to the shaders in the push_data block; for Metal an argument buffer is filled with the appropriate resources.

## Dependencies
DarkMatter uses GLFW for platform handling. It also comes with DearImgui bundled, specifically with the dear_bindings C bindings. Due to the bindless nature of DarkMatter, we DO NOT USE the render backends provided by DearImgui, only the GLFW backend. As of now, the user is responsible for creating and handling the imgui rendering using the render API. 
