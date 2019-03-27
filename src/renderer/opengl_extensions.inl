#ifdef EXTENSION_CONSTANTS

#else

#ifdef FUNCTIONLESS_EXTENSIONS
// Extensions which do not introduce any new functions
// GL_EXTENSION_NAME(EXT_REQUIRED / EXT_OPTIONAL, NAME)

GL_EXTENSION_NAME(EXT_REQUIRED, GL_ARB_texture_cube_map)
#else
// Extensions which do introduce new functions
// GL_EXTENSION_BEGIN(EXT_REQUIRED / EXT_OPTIONAL, EXTENSION_NAME)
// GL_EXTENSION_FUNC(RETURN TYPE, NAME, PARAMETERS)
// GL_EXTENSION_END()

GL_EXTENSION_BEGIN(EXT_REQUIRED, GL_ARB_texture_compression)
GL_EXTENSION_FUNC(void, glCompressedTexImage2DARB, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
GL_EXTENSION_FUNC(void, glCompressedTexSubImage2DARB, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
GL_EXTENSION_FUNC(void, glGetCompressedTexImageARB, GLenum target, GLint lod, void *img);
GL_EXTENSION_END()
#endif
#endif