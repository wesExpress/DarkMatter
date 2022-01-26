# CEngine

This is a cross-platform rendering engine for the purpose of learning. It is written entirely in C and does not use external libraries, with two main exceptions:
- Libraries
  - GLFW for prototyping with OpenGL
  - GLAD to access OpenGL functions
  - stb_image to load image data
- Objective C
  - for OSX specific code and metal rendering

The general design philosophy is as follows:
- Get a process of the rendering pipeline working with OpenGL
- Duplicate the same results on Windows with DirectX11 and on MacOSX with Metal
- Iterate

Features
- dm_list - dynamic sized array of generic type
- dm_map  - dynamic sized hash table of generic type

Current status:
- Textures are rendered on quads in both opengl and directx11
- Just starting to get a window and metal context working with Cocoa and Metal on OSX
