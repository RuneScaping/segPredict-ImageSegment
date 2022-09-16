/*
** The OpenGL Extension Wrangler Library
** Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
** Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
** Copyright (C) 2002, Lev Povalahev
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice, 
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution.
** * The name of the author may be used to endorse or promote products 
**   derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Mesa 3-D graphics library
 * Version:  7.0
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
** Copyright (c) 2007 The Khronos Group Inc.
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
** 
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef __glew_h__
#define __glew_h__
#define __GLEW_H__

#if defined(__gl_h_) || defined(__GL_H__)
#error gl.h included before glew.h
#endif
#if defined(__glext_h_) || defined(__GLEXT_H_)
#error glext.h included before glew.h
#endif
#if defined(__gl_ATI_h_)
#error glATI.h included before glew.h
#endif

#define __gl_h_
#define __GL_H__
#define __glext_h_
#define __GLEXT_H_
#define __gl_ATI_h_

#if defined(_WIN32)

/*
 * GLEW does not include <windows.h> to avoid name space pollution.
 * GL needs GLAPI and GLAPIENTRY, GLU needs APIENTRY, CALLBACK, and wchar_t
 * defined properly.
 */
/* <windef.h> */
#ifndef APIENTRY
#define GLEW_APIENTRY_DEFINED
#  if defined(__MINGW32__)
#    define APIENTRY __stdcall
#  elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif
#ifndef GLAPI
#  if defined(__MINGW32__)
#    define GLAPI extern
#  endif
#endif
/* <winnt.h> */
#ifndef CALLBACK
#define GLEW_CALLBACK_DEFINED
#  if defined(__MINGW32__)
#    define CALLBACK __attribute__ ((__stdcall__))
#  elif (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#    define CALLBACK __stdcall
#  else
#    define CALLBACK
#  endif
#endif
/* <wingdi.h> and <winnt.h> */
#ifndef WINGDIAPI
#define GLEW_WINGDIAPI_DEFINED
#define WINGDIAPI __declspec(dllimport)
#endif
/* <ctype.h> */
#if (defined(_MSC_VER) || defined(__BORLANDC__)) && !defined(_WCHAR_T_DEFINED)
typedef unsigned short wchar_t;
#  define _WCHAR_T_DEFINED
#endif
/* <stddef.h> */
#if !defined(_W64)
#  if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#    define _W64 __w64
#  else
#    define _W64
#  endif
#endif
#if !defined(_PTRDIFF_T_DEFINED) && !defined(_PTRDIFF_T_)
#  ifdef _WIN64
typedef __int64 ptrdiff_t;
#  else
typedef _W64 int ptrdiff_t;
#  endif
#  define _PTRDIFF_T_DEFINED
#  define _PTRDIFF_T_
#endif

#ifndef GLAPI
#  if defined(__MINGW32__)
#    define GLAPI extern
#  else
#    define GLAPI WINGDIAPI
#  endif
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

/*
 * GLEW_STATIC needs to be set when using the static version.
 * GLEW_BUILD is set when building the DLL version.
 */
#ifdef GLEW_STATIC
#  define GLEWAPI extern
#else
#  ifdef GLEW_BUILD
#    define GLEWAPI extern __declspec(dllexport)
#  else
#    define GLEWAPI extern __declspec(dllimport)
#  endif
#endif

#else /* _UNIX */

/*
 * Needed for ptrdiff_t in turn needed by VBO.  This is defined by ISO
 * C.  On my system, this amounts to _3 lines_ of included code, all of
 * them pretty much harmless.  If you know of a way of detecting 32 vs
 * 64 _targets_ at compile time you are free to replace this with
 * something that's portable.  For now, _this_ is the portable solution.
 * (mem, 2004-01-04)
 */

#include <stddef.h>

#define GLEW_APIENTRY_DEFINED
#define APIENTRY
#define GLEWAPI extern

/* <glu.h> */
#ifndef GLAPI
#define GLAPI extern
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- GL_VERSION_1_1 ---------------------------- */

#ifndef GL_VERSION_1_1
#define GL_VERSION_1_1 1

#if defined(__APPLE__)
typedef unsigned long GLenum;
typedef unsigned long GLbitfield;
typedef unsigned long GLuint;
typedef long GLint;
typedef long GLsizei;
#else
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
#endif
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned long GLulong;
typedef float GLfloat;
typedef float GLcl