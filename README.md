# DarkMatter

Cross platform and rendering framework.

Supports:

- Vulkan
- Metal

## Resources
DarkMatter uses the following resource types:

- Buffer:  generic container of bytes on the gpu. Further specify vertex, index, or storage
- Texture: right now just 2D supported. Can be sampled or storage
- Render Target: contains a color texture and details on how to load/clear. Specify swapchain for default
- Sampler: used for sampled textures

As DarkMatter uses bindless, it is the user's responsibility to determine how each resource should be used. E.G.: if you have a sampled texture, you won't be able to push it to a compute/ray tracing shader to read/write on.

## Commands
Basic commands are abstracted away. Combined with compute pipelines and render targets, modern rendering techniques should be quite simple to implement.

## Dependencies
DarkMatter uses GLFW for platform handling. It also comes with DearImgui bundled, specifically with the dear_bindings C bindings. Due to the bindless nature of DarkMatter, we DO NOT USE the render backends provided by DearImgui, only the GLFW backend. As of now, the user is responsible for creating and handling the imgui rendering using the render API. 
