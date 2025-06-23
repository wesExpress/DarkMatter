# DarkMatter

This is a cross-platform rendering framework for the purpose of learning. It is written entirely in C and does not use external libraries, with a few main exceptions:
- Libraries
  - stb libraries to handle image, font, sound file loading
- Objective C
  - for OSX specific code and Metal renderer
  - this means you need to include 'dm_platform_mac.m' and 'dm_renderer_metal.m' when building!
  
## Framework Goals
- Render backend agnostic:
  - DirectX 12
  - Metal
  - Vulkan
- Platform backend agnostic:
  - Windows
  - MacOS
  - TBD: Linux

## Current Progress
DarkMatter is only DX12 currently; this will change once ResourceDescriptorHeap is supported with Vulkan, or I find the time to square the circle of supporting the two vastly different approaches.

With DX12:
- raster pipelines
- raytracing pipelines
- bindless rendering
