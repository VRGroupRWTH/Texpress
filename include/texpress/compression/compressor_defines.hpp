#pragma once
#include <texpress/defines.hpp>
#include <glbinding/gl45core/enum.h>

#define TEXPRESS_BC6H_SIGNED gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#define TEXPRESS_BC6H gl::GLenum::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#define TEXPRESS_BC7 gl::GLenum::GL_COMPRESSED_RGBA_BPTC_UNORM

#define TEXPRESS_BYTE gl::GLenum::GL_BYTE
#define TEXPRESS_UBYTE gl::GLenum::GL_UNSIGNED_BYTE
#define TEXPRESS_INT gl::GLenum::GL_INT
#define TEXPRESS_UINT gl::GLenum::GL_UNSIGNED_INT
#define TEXPRESS_FLOAT gl::GLenum::GL_FLOAT
#define TEXPRESS_DOUBLE gl::GLenum::GL_DOUBLE