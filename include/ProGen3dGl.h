#pragma once

#include <glad/gl.h>

inline bool progen3d_initialize_gl_loader(GLADloadfunc load)
{
	return gladLoadGL(load) != 0;
}

inline bool progen3d_initialize_gl_loader_from_current_context()
{
	return gladLoaderLoadGL() != 0;
}

inline bool progen3d_texture_anisotropy_supported()
{
	return GLAD_GL_ARB_texture_filter_anisotropic != 0 ||
	       GLAD_GL_EXT_texture_filter_anisotropic != 0;
}
