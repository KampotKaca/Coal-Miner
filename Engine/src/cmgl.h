#ifndef CMGL_H
#define CMGL_H

#include <stdbool.h>

typedef enum CullingMode
{
	CM_CULL_FACE_FRONT = 0,
	CM_CULL_FACE_BACK
} CullingMode;

typedef enum ColorMask
{
	CM_RED = 1 << 0,
	CM_GREEN = 1 << 1,
	CM_BLUE = 1 << 2,
	CM_ALPHA = 1 << 3
} ColorMask;

const char *get_pixel_format_name(unsigned int format);
unsigned int load_texture(const void *data, int width, int height, int format, int mipmapCount);
void get_gl_texture_formats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType);
unsigned int compile_shader(const char *shaderCode, int type);
unsigned int load_shader_program(unsigned int vShaderId, unsigned int fShaderId);
void unload_shader_program(unsigned int id);

void upload_ubos();
void unload_ubos();

void set_line_width(float width);
float get_line_width(void);
void enable_smooth_lines(void);
void disable_smooth_lines(void);

#endif //CMGL_H
