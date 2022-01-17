# CEngine

This is a cross-platform rendering engine for the purpose of learning.

The general design philosophy is as follows:
- Get a process of the rendering pipeline working with OpenGL
- Duplicate the same results on Windows with DirectX11 and on MacOSX with Metal
- Iterate

Current status:
- Rendering quad and clearing screen with OpenGL
- Clearing screen with DirectX but **rendering isn't working yet**
- Just starting to get a window and metal context working with Cocoa and Metal on OSX
