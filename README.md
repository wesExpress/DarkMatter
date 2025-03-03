# DarkMatter

This is a cross-platform rendering framework for the purpose of learning. It is written entirely in C and does not use external libraries, with a few main exceptions:
- Libraries
  - stb libraries to handle image, font, sound file loading
- Objective C
  - for OSX specific code and Metal renderer
  - this means you need to include 'dm_platform_mac.m' and 'dm_renderer_metal.m' when building!
  
## Framework  
- Render backend agnostic:
  - DirectX 12
  - Metal
  - Vulkan
- Platform backend agnostic:
  - Windows
  - MacOS
  - TBD: Linux

## Current Progress
DarkMatter has parity between with DX12 and Vulkan with:
  - Descriptors: uses DescriptorBuffer with Vulkan to mimic ID3D12DescriptorHeap. Current implementation isn't great, but can handle uniforms and textures between both
  - Raster Pipelines: basic raster pipeline features handled between both. Still needs some configuration with blending, depth stencil, etc.

## Future Goals
- Raytracing pipelines between both
- Better texture handling

## Weird Bugs
- Vulkan has incorrect colors for some reason
