#pragma once

// Minimal OpenGL 3.3 core function loader.
//
// Function pointers are resolved through SDL_GL_GetProcAddress after a context has been created,
// so the engine does not link against libGL and works with both GLX and EGL (X11 / Wayland).
// glcorearb.h only provides types, enums and PFN typedefs (no prototypes), so there is no clash
// with the pointers declared here. Add new functions to GL_FUNCTION_LIST as they are needed.

#include <GL/glcorearb.h>

#define GL_FUNCTION_LIST(X)                                          \
    X(PFNGLGETSTRINGPROC,               glGetString)                 \
    X(PFNGLGETERRORPROC,                glGetError)                  \
    X(PFNGLVIEWPORTPROC,                glViewport)                  \
    X(PFNGLCLEARCOLORPROC,              glClearColor)                \
    X(PFNGLCLEARPROC,                   glClear)                     \
    X(PFNGLENABLEPROC,                  glEnable)                    \
    X(PFNGLDISABLEPROC,                 glDisable)                   \
    X(PFNGLBLENDFUNCPROC,               glBlendFunc)                 \
    X(PFNGLPIXELSTOREIPROC,             glPixelStorei)               \
    X(PFNGLCREATESHADERPROC,            glCreateShader)              \
    X(PFNGLSHADERSOURCEPROC,            glShaderSource)              \
    X(PFNGLCOMPILESHADERPROC,           glCompileShader)             \
    X(PFNGLGETSHADERIVPROC,             glGetShaderiv)               \
    X(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog)          \
    X(PFNGLDELETESHADERPROC,            glDeleteShader)              \
    X(PFNGLCREATEPROGRAMPROC,           glCreateProgram)             \
    X(PFNGLATTACHSHADERPROC,            glAttachShader)              \
    X(PFNGLLINKPROGRAMPROC,             glLinkProgram)               \
    X(PFNGLGETPROGRAMIVPROC,            glGetProgramiv)              \
    X(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog)         \
    X(PFNGLDELETEPROGRAMPROC,           glDeleteProgram)             \
    X(PFNGLUSEPROGRAMPROC,              glUseProgram)                \
    X(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation)        \
    X(PFNGLUNIFORM1IPROC,               glUniform1i)                 \
    X(PFNGLUNIFORM2FPROC,               glUniform2f)                 \
    X(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays)           \
    X(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray)           \
    X(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays)        \
    X(PFNGLGENBUFFERSPROC,              glGenBuffers)                \
    X(PFNGLBINDBUFFERPROC,              glBindBuffer)                \
    X(PFNGLBUFFERDATAPROC,              glBufferData)                \
    X(PFNGLBUFFERSUBDATAPROC,           glBufferSubData)             \
    X(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers)             \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)   \
    X(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer)       \
    X(PFNGLGENTEXTURESPROC,             glGenTextures)               \
    X(PFNGLBINDTEXTUREPROC,             glBindTexture)               \
    X(PFNGLDELETETEXTURESPROC,          glDeleteTextures)            \
    X(PFNGLACTIVETEXTUREPROC,           glActiveTexture)             \
    X(PFNGLTEXIMAGE2DPROC,              glTexImage2D)                \
    X(PFNGLTEXPARAMETERIPROC,           glTexParameteri)             \
    X(PFNGLDRAWELEMENTSPROC,            glDrawElements)

// The pointers live in their own namespace so their linker symbols never collide with the
// real entry points exported by the GL driver library.
namespace glfn {
#define GL_DECLARE_FUNCTION(type, name) extern type name;
GL_FUNCTION_LIST(GL_DECLARE_FUNCTION)
#undef GL_DECLARE_FUNCTION
}
using namespace glfn;

// Resolves every function in GL_FUNCTION_LIST. Requires a current GL context.
// Throws std::runtime_error naming the first missing function.
void gl_load_functions();
