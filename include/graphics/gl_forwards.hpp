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

using GLfloat = float;
using GLuint = unsigned int;
