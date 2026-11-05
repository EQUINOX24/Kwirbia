#include "gl_loader.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace glfn {
#define GL_DEFINE_FUNCTION(type, name) type name = nullptr;
GL_FUNCTION_LIST(GL_DEFINE_FUNCTION)
#undef GL_DEFINE_FUNCTION
}

void gl_load_functions()
{
#define GL_LOAD_FUNCTION(type, name)                                                    \
    glfn::name = reinterpret_cast<type>(SDL_GL_GetProcAddress(#name));                   \
    if (!glfn::name) {                                                                   \
        throw std::runtime_error(std::string("Missing OpenGL function: ") + #name);      \
    }
    GL_FUNCTION_LIST(GL_LOAD_FUNCTION)
#undef GL_LOAD_FUNCTION
}
