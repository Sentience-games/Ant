#ifdef EXTENSION_CONSTANTS
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3
#else

#ifdef FUNCTIONLESS_EXTENSIONS
// Extensions which do not introduce any new functions
// GL_EXTENSION_NAME(EXT_REQUIRED / EXT_OPTIONAL, NAME)
#else
// Extensions which do introduce new functions
// GL_EXTENSION_BEGIN(EXT_REQUIRED / EXT_OPTIONAL, EXTENSION_NAME)
// GL_EXTENSION_FUNC(RETURN TYPE, NAME, PARAMETERS)
// GL_EXTENSION_END()

GL_EXTENSION_BEGIN(EXT_REQUIRED, GL_ARB_texture_compression)
GL_EXTENSION_FUNC(void, glCompressedTexImage2DARB, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
GL_EXTENSION_FUNC(void, glCompressedTexSubImage2DARB, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
GL_EXTENSION_END()

GL_EXTENSION_BEGIN(EXT_REQUIRED, GL_ARB_bindless_texture)
GL_EXTENSION_FUNC(GLuint64, glGetTextureHandleARB,             GLuint texture)
GL_EXTENSION_FUNC(GLuint64, glGetTextureSamplerHandleARB,      GLuint texture, GLuint sampler)
GL_EXTENSION_FUNC(void,     glMakeTextureHandleResidentARB,    GLuint64 handle)
GL_EXTENSION_FUNC(void,     glMakeTextureHandleNonResidentARB, GLuint64 handle)
GL_EXTENSION_END()
#endif
#endif