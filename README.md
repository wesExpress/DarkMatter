# DarkMatter

This is a cross-platform rendering framework for the purpose of learning. It is written entirely in C and does not use external libraries, with a few main exceptions:
- Libraries
  - GLAD to access OpenGL functions
  - stb libraries to handle image, font, sound file loading
- Objective C
  - for OSX specific code and metal renderer
  - this means you need to include 'dm_platform_mac.m' and 'dm_renderer_metal.m' when building!
  
This framework is being built with the following philosophy:
- **UNDERSTANDING IS KING:** I am most concerned that someone would be able to easily read/follow the code. If that results in sub-optimal code, then so be it; I am not writing this to be generally used by people. This is primarily a result of stumbling across too many repos where the code is virtually unreadable, with either functions/definitions behind multiple function calls/headers, or the code is a mess of modern c++ template vomit.

## Features
- Opengl, DirectX11, Metal renering backends
  - Render command setup, backend agnostic
- Windows, Linux, OSX platform specific window/input handling
- Rudimentary ECS framework
  - Basically a wrapper around a structure of arrays
- Initial implementation of full 3D physics collision and resolution system
  - Sort and sweep on AABB for broadphase
  - GJK, EPA, collision manifolds, constraint Jacobian solving
