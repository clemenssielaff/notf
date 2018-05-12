#pragma once

#include "common/forwards.hpp"

/// Lightweight forward declaration of OpenGL types.
///
/// Generally speaking, this is not safe as the GL* types could be defined to something different on another machine.
/// We can make it safe, however, by including this file into the _header_ (hence the 'forward'-part of the file name),
/// and include the real OpenGL library into the source (which is necessary to do anything with these types anyway).
/// If the definitions of the GL* types vary from this header file to the types declared by OpenGL, the compiler will
/// complain - if it compiles on the other hand, it should be safe to assume that everything is fine.

// no namespace

using GLenum     = unsigned int;
using GLboolean  = unsigned char;
using GLbitfield = unsigned int;
using GLvoid     = void;
using GLbyte     = signed char;
using GLshort    = short;
using GLint      = int;
using GLintptr   = long int;
using GLclampx   = int;
using GLubyte    = unsigned char;
using GLushort   = unsigned short;
using GLuint     = unsigned int;
using GLsizei    = int;
using GLsizeiptr = long int;
using GLfloat    = float;
using GLclampf   = float;
using GLdouble   = double;
using GLclampd   = double;
using GLchar     = char;
using GLcharARB  = char;

NOTF_OPEN_NAMESPACE

NOTF_DEFINE_SHARED_POINTERS(class, Font);
NOTF_DEFINE_SHARED_POINTERS(class, FragmentShader);
NOTF_DEFINE_SHARED_POINTERS(class, FrameBuffer);
NOTF_DEFINE_SHARED_POINTERS(class, GeometryShader);
NOTF_DEFINE_SHARED_POINTERS(class, Pipeline);
NOTF_DEFINE_SHARED_POINTERS(class, RenderBuffer);
NOTF_DEFINE_SHARED_POINTERS(class, Shader);
NOTF_DEFINE_SHARED_POINTERS(class, TesselationShader);
NOTF_DEFINE_SHARED_POINTERS(class, Texture);
NOTF_DEFINE_SHARED_POINTERS(class, VertexShader);

NOTF_DEFINE_UNIQUE_POINTERS(class, GraphicsContext);
NOTF_DEFINE_UNIQUE_POINTERS(class, VertexArrayType);
NOTF_DEFINE_UNIQUE_POINTERS(class, IndexArrayType);
NOTF_DEFINE_UNIQUE_POINTERS(class, FontManager);

NOTF_CLOSE_NAMESPACE
