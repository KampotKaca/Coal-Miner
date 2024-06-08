#ifndef COAL_GL_H
#define COAL_GL_H

const char *get_pixel_format_name(unsigned int format);
unsigned int load_texture(const void *data, int width, int height, int format, int mipmapCount);
void get_gl_texture_formats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType);

#endif //COAL_GL_H
