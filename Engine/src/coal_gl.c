#include "coal_gl.h"
#include <glad/glad.h>
#include "coal_image.h"
#include "coal_miner.h"

static int GetPixelDataSize(int width, int height, int format);

const char *get_pixel_format_name(unsigned int format)
{
	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: return "GRAYSCALE";         // 8 bit per pixel (no alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: return "GRAY_ALPHA";       // 8*2 bpp (2 channels)
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5: return "R5G6B5";               // 16 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: return "R8G8B8";               // 24 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1: return "R5G5B5A1";           // 16 bpp (1 bit alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: return "R4G4B4A4";           // 16 bpp (4 bit alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: return "R8G8B8A8";           // 32 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R32: return "R32";                     // 32 bpp (1 channel - float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: return "R32G32B32";         // 32*3 bpp (3 channels - float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: return "R32G32B32A32";   // 32*4 bpp (4 channels - float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R16: return "R16";                     // 16 bpp (1 channel - half float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16: return "R16G16B16";         // 16*3 bpp (3 channels - half float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16: return "R16G16B16A16";   // 16*4 bpp (4 channels - half float)
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB: return "DXT1_RGB";             // 4 bpp (no alpha)
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA: return "DXT1_RGBA";           // 4 bpp (1 bit alpha)
		case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA: return "DXT3_RGBA";           // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA: return "DXT5_RGBA";           // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB: return "ETC1_RGB";             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB: return "ETC2_RGB";             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA: return "ETC2_RGBA";       // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB: return "PVRT_RGB";             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: return "PVRT_RGBA";           // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: return "ASTC_4x4_RGBA";   // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: return "ASTC_8x8_RGBA";   // 2 bpp
		default: return "UNKNOWN";
	}
}

unsigned int load_texture(const void *data, int width, int height, int format, int mipmapCount)
{
	unsigned int id = 0;
	
	glBindTexture(GL_TEXTURE_2D, 0);    // Free any old binding
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &id);              // Generate texture id
	glBindTexture(GL_TEXTURE_2D, id);
	
	int mipWidth = width;
	int mipHeight = height;
	int mipOffset = 0;          // Mipmap data offset, only used for tracelog
	
	// NOTE: Added pointer math separately from function to avoid UBSAN complaining
	unsigned char *dataPtr = NULL;
	if (data != NULL) dataPtr = (unsigned char *)data;
	
	// Load the different mipmap levels
	for (int i = 0; i < mipmapCount; i++)
	{
		unsigned int mipSize = GetPixelDataSize(mipWidth, mipHeight, format);
		
		unsigned int glInternalFormat, glFormat, glType;
		get_gl_texture_formats(format, &glInternalFormat, &glFormat, &glType);
		
		printf("TEXTURE: Load mipmap level %i (%i x %i), size: %i, offset: %i", i, mipWidth, mipHeight, mipSize, mipOffset);
		
		if (glInternalFormat != 0)
		{
			if (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) glTexImage2D(GL_TEXTURE_2D, i, (int)glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, dataPtr);
			if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
			else if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
		}
		
		mipWidth /= 2;
		mipHeight /= 2;
		mipOffset += (int)mipSize;       // Increment offset position to next mipmap
		if (data != NULL) dataPtr += mipSize;         // Increment data pointer to next mipmap
		
		// Security check for NPOT textures
		if (mipWidth < 1) mipWidth = 1;
		if (mipHeight < 1) mipHeight = 1;
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // Set texture to repeat on y-axis
	
	// Magnification and minification filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
	
	if (mipmapCount > 1)
	{
		// Activate Trilinear filtering if mipmaps are available
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	
	// At this point we have the texture loaded in GPU and texture parameters configured
	
	// NOTE: If mipmaps were not in data, they are not generated automatically
	
	// Unbind current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	
	if (id > 0) printf("TEXTURE: [ID %i] Texture loaded successfully (%ix%i | %s | %i mipmaps)", id, width, height, get_pixel_format_name(format), mipmapCount);
	else printf("TEXTURE: Failed to load texture");
	
	return id;
}

void get_gl_texture_formats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType)
{
	*glInternalFormat = 0;
	*glFormat = 0;
	*glType = 0;
	
	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: *glInternalFormat = GL_R8; *glFormat = GL_RED; *glType = GL_UNSIGNED_BYTE; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: *glInternalFormat = GL_RG8; *glFormat = GL_RG; *glType = GL_UNSIGNED_BYTE; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5: *glInternalFormat = GL_RGB565; *glFormat = GL_RGB; *glType = GL_UNSIGNED_SHORT_5_6_5; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: *glInternalFormat = GL_RGB8; *glFormat = GL_RGB; *glType = GL_UNSIGNED_BYTE; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1: *glInternalFormat = GL_RGB5_A1; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_5_5_5_1; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: *glInternalFormat = GL_RGBA4; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_4_4_4_4; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: *glInternalFormat = GL_RGBA8; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_BYTE; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32: *glInternalFormat = GL_R32F; *glFormat = GL_RED; *glType = GL_FLOAT; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: *glInternalFormat = GL_RGB32F; *glFormat = GL_RGB; *glType = GL_FLOAT; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: *glInternalFormat = GL_RGBA32F; *glFormat = GL_RGBA; *glType = GL_FLOAT; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16: *glInternalFormat = GL_R16F; *glFormat = GL_RED; *glType = GL_HALF_FLOAT; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16: *glInternalFormat = GL_RGB16F; *glFormat = GL_RGB; *glType = GL_HALF_FLOAT; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16: *glInternalFormat = GL_RGBA16F; *glFormat = GL_RGBA; *glType = GL_HALF_FLOAT; break;
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB: *glInternalFormat = GL_COMPRESSED_RGB8_ETC2; break;               // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA: *glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC; break;     // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		
		default: printf("TEXTURE: Current format not supported (%i)", format); break;
	}
}

// Get pixel data size in bytes (image or texture)
// NOTE: Size depends on pixel format
static int GetPixelDataSize(int width, int height, int format)
{
	int dataSize;       // Size in bytes
	int bpp = 0;            // Bits per pixel
	
	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: bpp = 8; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5:
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1:
		case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: bpp = 16; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: bpp = 32; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: bpp = 24; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32: bpp = 32; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: bpp = 32*3; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: bpp = 32*4; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R16: bpp = 16; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16: bpp = 16*3; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16: bpp = 16*4; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB:
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB:
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB:
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB:
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: bpp = 4; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: bpp = 8; break;
		case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: bpp = 2; break;
		default: break;
	}
	
	dataSize = width*height*bpp/8;  // Total data size in bytes
	
	// Most compressed formats works on 4x4 blocks,
	// if texture is smaller, minimum dataSize is 8 or 16
	if ((width < 4) && (height < 4))
	{
		if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) && (format < RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA)) dataSize = 8;
		else if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA) && (format < RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA)) dataSize = 16;
	}
	
	return dataSize;
}

Shader cm_load_shader(const char *vsPath, const char *fsPath)
{
	Shader shader;

	char *vShaderStr = NULL;
	char *fShaderStr = NULL;

	if (vsPath != NULL) vShaderStr = cm_load_file_text(vsPath);
	if (fsPath != NULL) fShaderStr = cm_load_file_text(fsPath);

	shader = cm_load_shader_from_memory(vShaderStr, fShaderStr);

	cm_unload_file_text(vShaderStr);
	cm_unload_file_text(fShaderStr);

	return shader;
}

Shader cm_load_shader_from_memory(const char *vsCode, const char *fsCode)
{
	Shader shader = { 0 };

	unsigned int vertexShaderId = 0;
    unsigned int fragmentShaderId = 0;

    // Compile vertex shader (if provided)
    if (vsCode != NULL) vertexShaderId = compile_shader(vsCode, GL_VERTEX_SHADER);

    // Compile fragment shader (if provided)
    if (fsCode != NULL) fragmentShaderId = compile_shader(fsCode, GL_FRAGMENT_SHADER);

	// One of or both shader are new, we need to compile a new shader program
	shader.id = load_shader_program(vertexShaderId, fragmentShaderId);

	// Get available shader uniforms
	// NOTE: This information is useful for debug...
	int uniformCount = -1;
	glGetProgramiv(shader.id, GL_ACTIVE_UNIFORMS, &uniformCount);

	for (int i = 0; i < uniformCount; i++)
	{
		int namelen = -1;
		int num = -1;
		char name[256] = { 0 };     // Assume no variable names longer than 256
		GLenum type = GL_ZERO;

		// Get the name of the uniforms
		glGetActiveUniform(shader.id, i, sizeof(name) - 1, &namelen, &num, &type, name);

		name[namelen] = 0;
		printf("SHADER: [ID %i] Active uniform (%s) set at location: %i", shader.id, name, glGetUniformLocation(shader.id, name));
	}

	// We can detach and delete vertex/fragment shaders (if not default ones)
	// NOTE: We detach shader before deletion to make sure memory is freed
	if (vertexShaderId != 0)
	{
		// WARNING: Shader program linkage could fail and returned id is 0
		if (shader.id > 0) glDetachShader(shader.id, vertexShaderId);
		glDeleteShader(vertexShaderId);
	}
	if (fragmentShaderId != 0)
	{
		// WARNING: Shader program linkage could fail and returned id is 0
		if (shader.id > 0) glDetachShader(shader.id, fragmentShaderId);
		glDeleteShader(fragmentShaderId);
	}

	return shader;
}

void cm_unload_shader(Shader shader)
{
	if (shader.id != 0)
	{
		unload_shader_program(shader.id);
		// NOTE: If shader loading failed, it should be 0
		CM_FREE(shader.locs);
	}
}

unsigned int compile_shader(const char *shaderCode, int type)
{
	unsigned int shader;

	shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, NULL);

    GLint success = 0;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        switch (type)
        {
            case GL_VERTEX_SHADER: printf("SHADER: [ID %i] Failed to compile vertex shader code", shader); break;
            case GL_FRAGMENT_SHADER: printf("SHADER: [ID %i] Failed to compile fragment shader code", shader); break;
            //case GL_GEOMETRY_SHADER:
            case GL_COMPUTE_SHADER: printf("SHADER: [ID %i] Failed to compile compute shader code", shader); break;
            default: break;
        }

        int maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            char *log = (char *)CM_CALLOC(maxLength, sizeof(char));
            glGetShaderInfoLog(shader, maxLength, &length, log);
            printf("SHADER: [ID %i] Compile error: %s", shader, log);
            CM_FREE(log);
        }
    }
    else
    {
        switch (type)
        {
            case GL_VERTEX_SHADER: printf("SHADER: [ID %i] Vertex shader compiled successfully", shader); break;
            case GL_FRAGMENT_SHADER: printf("SHADER: [ID %i] Fragment shader compiled successfully", shader); break;
            case GL_COMPUTE_SHADER: printf("SHADER: [ID %i] Compute shader compiled successfully", shader); break;
            default: break;
        }
    }

	return shader;
}

unsigned int load_shader_program(unsigned int vShaderId, unsigned int fShaderId)
{
	unsigned int program = 0;

	GLint success = 0;
    program = glCreateProgram();

    glAttachShader(program, vShaderId);
    glAttachShader(program, fShaderId);
    glLinkProgram(program);

    // NOTE: All uniform variables are intitialised to 0 when a program links

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        printf("SHADER: [ID %i] Failed to link shader program", program);

        int maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            char *log = (char *)CM_CALLOC(maxLength, sizeof(char));
            glGetProgramInfoLog(program, maxLength, &length, log);
	        printf("SHADER: [ID %i] Link error: %s", program, log);
            CM_FREE(log);
        }

        glDeleteProgram(program);

        program = 0;
    }
    else printf("SHADER: [ID %i] Program shader loaded successfully", program);
	return program;
}

void unload_shader_program(unsigned int id)
{
	glDeleteProgram(id);
}