#pragma once

/*
 * \file Lightweight forward declaration of OpenGL types.
 *
 * Generally speaking, this is not safe as the GL* types could be defined to something different on another machine.
 * We can make it safe, however, by including this file into the _header_ (hence the 'forward'-part of the file name),
 * and include the real OpenGL library into the source (which is necessary to do anything with these types anyway).
 * If the definitions of the GL* types vary from this header file to the types declared by OpenGL, the compiler will
 * complain - if it compiles on the other hand, it should be safe to assume that everything is fine.
 */

// no namespace

using GLenum     = unsigned int;
using GLboolean  = unsigned char;
using GLbitfield = unsigned int;
using GLvoid     = void;
using GLbyte     = signed char;
using GLshort    = short;
using GLint      = int;
using GLintptr   = long int; // this should differ on Windows
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

namespace notf {

/** Version of OpenGL in use. */
enum class OpenGLVersion : unsigned char {
    OPENGL_3,
    GLES_3,
};

} // namespace notf
