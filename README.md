# DarkMatter

This is a cross-platform rendering framework for the purpose of learning. It is written entirely in C and does not use external libraries, with a few main exceptions:
- Libraries
  - stb libraries to handle image, font, sound file loading
- Objective C
  - for OSX specific code and Metal renderer
  - this means you need to include 'dm_platform_mac.m' and 'dm_renderer_metal.m' when building!
  
## Features
- Render backend agnostic:
  - DirectX 11
  - DirectX 12
  - Metal
  - TBD: Vulkan
- Platform backend agnostic:
  - Windows
  - MacOS
  - TBD: Linux
- Initial implementation of full 3D physics collision and resolution
  - Sort and sweep on AABB for broadphase
  - GJK, EPA, collision manifolds, constraint Jacobian solving
- Rudimentary support for DXR
