The graphics/engine folder contains the basis for a graphics engine built on top of OpenGL ES 3.2.

The purpose of classes in this folder is to provide a toolbox of OpenGL abstractions that can be used to construct a
new, purpose-built graphics engine with minimal overhead.

The lowest-level object is the `Context`. The context is a minimal abstraction of the OpenGL context and should contain
most (if not all) of the OpenGL specific calls.
Apart from exposing OpenGL functionality, the context offers basic usablity features like state stacks and error
reporting and -handling.




On the highest level, an Engine object stores all relevant data in order to render a complete frame.
This includes:

    * uniform blocks
    * textures
    * shaders
    * prefab groups

A prefab group requires a shader with a matching attribute layout to be bound.

A it is the Pass' responsibility to make sure that its prefabs and shader share the same attribute layout.
A Pass consists of:
    1 draw call (instanced)
    attribute layout
    texture slot assignment
    framebuffer
All but the root Pass also have a `dirty` flag that can be used to re-use a previously rendered texture.

