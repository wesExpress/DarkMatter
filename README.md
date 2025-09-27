DarkMatter is a single header-file framework that abstracts the three main modern graphics APIs: DirectX12, Vulkan, and Metal. It acomplishes this through a simple abstraction layer where you create three main types of rendering resources:

1. Render Passes
2. Pipelines
3. Resources

and submit them to a command buffer through various commands:

1. Begin Render Pass
2. Bind Raster Pipeline
3. Bind Vertex Buffer
4. Draw Indexed

# Bindless Rendering
Each of the three backends utilize bindless rendering, which allows the user to focus on rendering and utilize the GPU better.

## DirectX12
The DirectX12 backend has only a push constant buffer in the root signature. This makes it very simple to access resources, by simply pushing indexes before draws. In shaders, you simply index into ResourceDescriptorHeap or SamplerDescriptorHeap to retrieve whatever resource you need.

## Vulkan
We use the VK_EXT_mutable_descriptor type to let us alias the first binding in shaders, almost mimicing the ResourceDescriptorHeap method from DX12.

## Metal
For Metal, we use argument buffers. To encode the argument buffers, the following render commands should be called in this order:
 - dm_render_command_bind_raster_pipeline(...)
 - dm_render_command_submit_resources(...)

Note that this is different from the other render backends; submit_resources can be called before or after bind_pipeline there.

### Rasterization Pipeline
The pipeline expects one argument buffer bound to buffer slot 0, and two argument buffers bound to slots 0 and 1 for the fragment shader. The first fragment argument buffer is dedicated to textures and samplers, and is filled with all of these resources before use, similar to a DX12 heap. These can simply be indexed into. The remaining argument buffers can hold whatever you define.

### Compute Pipeline

### Raytracing Pipeline
