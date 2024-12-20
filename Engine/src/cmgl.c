#include "cmgl.h"
#include <glad/glad.h>
#include "coal_image.h"

#define CHECK_GL_ERROR(...)                                     \
    {                                                           \
        GLenum err;                                             \
        while ((err = glGetError()) != GL_NO_ERROR) {           \
            log_log(LOG_ERROR, __FILE__, __LINE__,              \
					"[Open GL] error in %s: %s",                \
					__VA_ARGS__, gluErrorString(err));          \
            break;                                              \
        }                                                       \
    }

typedef struct Ubo
{
	bool isReady;
	char name[MAX_SHADER_UNIFORM_NAME_LENGTH];
	uint32_t id;
	uint32_t bindingId;
	uint32_t dataSize;
	const void* data;
} Ubo;

Ubo CM_UBOS[MAX_NUM_UBOS];
uint32_t cmUboCount;

static int GetPixelDataSize(int width, int height, int format);

const char *get_pixel_format_name(uint32_t format)
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

uint32_t load_texture(const void *data, int width, int height,
						  TextureFlags wrap, TextureFlags filter, int format, int mipmapCount)
{
	uint32_t id = 0;
	
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
		uint32_t mipSize = GetPixelDataSize(mipWidth, mipHeight, format);
		
		uint32_t glInternalFormat, glFormat, glType;
		get_gl_texture_formats(format, &glInternalFormat, &glFormat, &glType);

		log_trace("TEXTURE: Load mipmap level %i (%i x %i), size: %i, offset: %i", i, mipWidth, mipHeight, mipSize, mipOffset);
		
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
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);       // Set texture to repeat on x-axis
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);       // Set texture to repeat on y-axis
	
	if(filter != CM_TEXTURE_FILTER_TRILINEAR)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	}
	else
	{
		// Magnification and minification filters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
		
		if (mipmapCount > 1)
		{
			// Activate Trilinear filtering if mipmaps are available
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
	}
	
	// At this point we have the texture loaded in GPU and texture parameters configured
	
	// NOTE: If mipmaps were not in data, they are not generated automatically
	
	// Unbind current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	
	if (id > 0) log_trace("TEXTURE: [ID %i] Texture loaded successfully (%ix%i | %s | %i mipmaps)", id, width, height, get_pixel_format_name(format), mipmapCount);
	else log_warn("TEXTURE: Failed to load texture");
	
	return id;
}

void cm_unload_texture(Texture tex)
{
	glDeleteTextures(1, &tex.id);
}

void get_gl_texture_formats(int format, uint32_t *glInternalFormat, uint32_t *glFormat, uint32_t *glType)
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
		
		default: log_warn("TEXTURE: Current format not supported (%i)", format); break;
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

	uint32_t vertexShaderId = 0;
    uint32_t fragmentShaderId = 0;

    // Compile vertex shader (if provided)
    if (vsCode != NULL) vertexShaderId = compile_shader(vsCode, GL_VERTEX_SHADER);

    // Compile fragment shader (if provided)
    if (fsCode != NULL) fragmentShaderId = compile_shader(fsCode, GL_FRAGMENT_SHADER);

	// One of or both shader are new, we need to compile a new shader program
	shader.id = load_shader_program(vertexShaderId, fragmentShaderId);
	shader.uniforms = list_create(0);
	
	// Get available shader uniforms
	int uniformCount = -1;
	glGetProgramiv(shader.id, GL_ACTIVE_UNIFORMS, &uniformCount);
	
	for (uint32_t i = 0; i < uniformCount; i++)
	{
		// Check if this uniform belongs to a UBO
		GLint blockIndex;
		glGetActiveUniformsiv(shader.id, 1, &i, GL_UNIFORM_BLOCK_INDEX, &blockIndex);

		if(blockIndex == -1)
		{
			ShaderUniform uniform = {  };
			glGetActiveUniform(shader.id, i, sizeof(uniform.name), &uniform.length, &uniform.size, &uniform.type, uniform.name);
			uniform.location = glGetUniformLocation(shader.id, uniform.name);
			
			list_add(&shader.uniforms, 1, &uniform, sizeof(ShaderUniform));
		}
	}
	
	for (int i = 0; i < cmUboCount; ++i)
	{
		// Retrieve the uniform block index corresponding to the binding point
		GLint blockIndex = glGetUniformBlockIndex(shader.id, CM_UBOS[i].name);
		
		if (blockIndex == GL_INVALID_INDEX) continue;
		
		// Bind the buffer to the specified binding point
		glUniformBlockBinding(shader.id, blockIndex, CM_UBOS[i].bindingId);
		glBindBufferBase(GL_UNIFORM_BUFFER, CM_UBOS[i].bindingId, CM_UBOS[i].id);
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
		list_clear(&shader.uniforms);
	}
}

uint32_t compile_shader(const char *shaderCode, int type)
{
	uint32_t shader;

	shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, NULL);

    GLint success = 0;
	glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        switch (type)
        {
            case GL_VERTEX_SHADER: log_error("SHADER: [ID %i] Failed to compile vertex shader code", shader); break;
            case GL_FRAGMENT_SHADER: log_error("SHADER: [ID %i] Failed to compile fragment shader code", shader); break;
            //case GL_GEOMETRY_SHADER:
            case GL_COMPUTE_SHADER: log_error("SHADER: [ID %i] Failed to compile compute shader code", shader); break;
            default: break;
        }

        int maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            char *log = (char *)CM_CALLOC(maxLength, sizeof(char));
            glGetShaderInfoLog(shader, maxLength, &length, log);
            log_error("SHADER: [ID %i] Compile error: %s", shader, log);
            CM_FREE(log);
        }
    }

	return shader;
}

uint32_t load_shader_program(uint32_t vShaderId, uint32_t fShaderId)
{
	uint32_t program;

	GLint success = 0;
    program = glCreateProgram();

    glAttachShader(program, vShaderId);
    glAttachShader(program, fShaderId);
    glLinkProgram(program);

    // NOTE: All uniform variables are intitialised to 0 when a program links

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE)
    {
        log_error("SHADER: [ID %i] Failed to link shader program", program);

        int maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            int length = 0;
            char *log = (char *)CM_CALLOC(maxLength, sizeof(char));
            glGetProgramInfoLog(program, maxLength, &length, log);
	        log_error("SHADER: [ID %i] Link error: %s", program, log);
            CM_FREE(log);
        }

        glDeleteProgram(program);

        program = 0;
    }
	
	return program;
}

void unload_shader_program(uint32_t id)
{
	glDeleteProgram(id);
}

Ssbo cm_load_ssbo(uint32_t bindingId, uint32_t dataSize, const void* data)
{
	Ssbo ssbo = {0};
	ssbo.bindingId = bindingId;
	ssbo.dataSize = dataSize;

	glGenBuffers(1, &ssbo.id);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo.id);

	if(data != NULL) glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo.dataSize, data, GL_STATIC_DRAW);
	else glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo.dataSize, data, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.bindingId, ssbo.id); // Bind to binding point 0
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return ssbo;
}

void cm_upload_ssbo(Ssbo ssbo, uint32_t offset, uint32_t size, const void* data)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo.id);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void cm_unload_ssbo(Ssbo ssbo)
{
	glDeleteBuffers(1, &ssbo.id);
}

bool cm_load_ubo(const char* name, uint32_t bindingId, uint32_t dataSize, const void* data)
{
	if(cmUboCount == MAX_NUM_UBOS)
	{
		log_error("%s", "Unable to create Ubo, max count was already reached");
		return false;
	}
	
	if(data == NULL)
	{
		log_error("%s", "Unable to create Ubo, data should never be NULL");
		return false;
	}
	
	Ubo ubo = {  };
	strcpy(ubo.name, name);
	ubo.dataSize = dataSize;
	ubo.data = data;
	ubo.bindingId = bindingId;
	
	glGenBuffers(1, &ubo.id);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo.id);
	glBufferData(GL_UNIFORM_BUFFER, dataSize, data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, ubo.bindingId, ubo.id);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	ubo.isReady = true;
	CM_UBOS[cmUboCount] = ubo;
	cmUboCount++;
	return true;
}

void cm_upload_ubos()
{
	for (int i = 0; i < cmUboCount; ++i)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, CM_UBOS[i].id);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, CM_UBOS[i].dataSize, CM_UBOS[i].data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void unload_ubos()
{
	for (int i = 0; i < cmUboCount; ++i)
	{
		glDeleteBuffers(1, &CM_UBOS[i].id);
		CM_UBOS[i].isReady = false;
	}
	cmUboCount = 0;
}

Vao cm_load_vao(VaoAttribute* attributes, uint32_t attributeCount, Vbo vbo)
{
	Vao vao = { 0 };
	vao.vbo = (Vbo){ 0 };
	vao.attributeCount = attributeCount;
	vao.attributes = CM_CALLOC(attributeCount, sizeof(VaoAttribute));
	vao.stride = 0;
	memcpy(vao.attributes, attributes, attributeCount * sizeof(VaoAttribute));
	glGenVertexArrays(1, &vao.id);
	glBindVertexArray(vao.id);

	vao.vbo = cm_load_vbo(vbo.dataSize, vbo.vertexCount, vbo.data, vbo.ebo);

	for (int i = 0; i < vao.attributeCount; ++i) vao.stride += vao.attributes[i].stride;

	uint32_t offset = 0;
	
	for (int i = 0; i < vao.attributeCount; ++i)
	{
		VaoAttribute attrib = vao.attributes[i];
		if(attrib.type < CM_HALF_FLOAT)
		{
			glVertexAttribIPointer(i, (int)attrib.size, attrib.type,
			                      (int)vao.stride, (void*)(uintptr_t)offset);
		}
		else
		{
			glVertexAttribPointer(i, (int)attrib.size,
			                      attrib.type,
			                      attrib.normalized,
			                      (int)vao.stride, (void*)(uintptr_t)offset);
		}
		
		offset += attrib.stride;
		glEnableVertexAttribArray(i);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return vao;
}

void cm_unload_vao(Vao vao)
{
	cm_unload_vbo(vao.vbo);
	glDeleteVertexArrays(1, &vao.id);
	CM_FREE(vao.attributes);
}

Vbo cm_load_vbo(uint32_t dataSize, uint32_t vertexCount, const void* data, Ebo ebo)
{
	Vbo vbo = { 0 };
	vbo.ebo = (Ebo){ 0 };
	vbo.dataSize = dataSize;
	vbo.vertexCount = vertexCount;
	vbo.data = data;
	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);

	if(vbo.data != NULL) glBufferData(GL_ARRAY_BUFFER, vbo.dataSize, vbo.data, GL_STATIC_DRAW);
	else glBufferData(GL_ARRAY_BUFFER, vbo.dataSize, vbo.data, GL_DYNAMIC_DRAW);

	if(ebo.dataSize > 0)
		vbo.ebo = cm_load_ebo(ebo.dataSize, ebo.data, ebo.type, ebo.indexCount);

	return vbo;
}

void cm_unload_vbo(Vbo vbo)
{
	cm_unload_ebo(vbo.ebo);
	glDeleteBuffers(1, &vbo.id);
}

void cm_reupload_vbo(Vbo* vbo, uint32_t dataSize, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo->id);
	vbo->dataSize = dataSize;
	vbo->data = data;
	glBufferData(GL_ARRAY_BUFFER, vbo->dataSize, vbo->data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cm_reupload_vbo_partial(Vbo* vbo, uint32_t dataOffset, uint32_t uploadSize)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo->id);
	glBufferSubData(GL_ARRAY_BUFFER, dataOffset, uploadSize, vbo->data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Ebo cm_load_ebo(uint32_t dataSize, const void* data, uint32_t type, uint32_t indexCount)
{
	Ebo ebo = { 0 };
	ebo.dataSize = dataSize;
	ebo.data = data;
	ebo.type = type;
	ebo.indexCount = indexCount;
	glGenBuffers(1, &ebo.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);

	if(ebo.data != NULL) glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo.dataSize, ebo.data, GL_STATIC_DRAW);
	else glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo.dataSize, ebo.data, GL_DYNAMIC_DRAW);

	return ebo;
}

void cm_unload_ebo(Ebo ebo)
{
	glDeleteBuffers(1, &ebo.id);
}

void cm_reupload_ebo(Ebo* ebo, uint32_t dataSize, const void* data, uint32_t indexCount)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo->id);
	ebo->dataSize = dataSize;
	ebo->data = data;
	ebo->indexCount = indexCount;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo->dataSize, ebo->data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

extern void cm_draw_vao(Vao vao, DrawType drawType)
{
	glBindVertexArray(vao.id);


	if(vao.vbo.ebo.dataSize == 0)
	{
		glDrawArrays(drawType, 0, (int)vao.vbo.vertexCount);
	}
	else
	{
		Ebo ebo = vao.vbo.ebo;
		glDrawElements(drawType, (int)ebo.indexCount, ebo.type, 0);
	}
}

void cm_draw_instanced_vao(Vao vao, DrawType drawType, uint32_t instanceCount)
{
	glBindVertexArray(vao.id);

	if(vao.vbo.ebo.dataSize == 0)
	{
		glDrawArraysInstanced(drawType, 0, (int)vao.vbo.vertexCount, (int)instanceCount);
	}
	else
	{
		Ebo ebo = vao.vbo.ebo;
		glDrawElementsInstanced(drawType, (int)ebo.indexCount, ebo.type, 0, (int)instanceCount);
	}
}

void cm_begin_shader_mode(Shader shader)
{
	glUseProgram(shader.id);
}

void cm_end_shader_mode()
{
	glUseProgram(0);
}

int cm_get_uniform_location(Shader shader, const char* name) { return glGetUniformLocation(shader.id, name); }

void cm_set_uniform_m4x4(int id, float* m) { glUniformMatrix4fv(id, 1, GL_FALSE, m); }

void cm_set_uniform_vec4(int id, float* m) { glUniform4f(id, m[0], m[1], m[2], m[3]); }
void cm_set_uniform_ivec4(int id, int* m) { glUniform4i(id, m[0], m[1], m[2], m[3]); }
void cm_set_uniform_uvec4(int id, uint32_t* m) { glUniform4ui(id, m[0], m[1], m[2], m[3]); }

void cm_set_uniform_vec3(int id, float* m) { glUniform3f(id, m[0], m[1], m[2]); }
void cm_set_uniform_ivec3(int id, int* m) { glUniform3i(id, m[0], m[1], m[2]); }
void cm_set_uniform_uvec3(int id, uint32_t* m) { glUniform3ui(id, m[0], m[1], m[2]); }

void cm_set_uniform_vec2(int id, float* m) { glUniform2f(id, m[0], m[1]); }
void cm_set_uniform_ivec2(int id, int* m) { glUniform2i(id, m[0], m[1]); }
void cm_set_uniform_uvec2(int id, uint32_t* m) { glUniform2ui(id, m[0], m[1]); }

void cm_set_uniform_f(int id, float f) { glUniform1f(id, f); }
void cm_set_uniform_i(int id, int f) { glUniform1i(id, f); }
void cm_set_uniform_u(int id, uint32_t f) { glUniform1ui(id, f); }

void cm_set_texture(int id, uint32_t texId, unsigned char bindingPoint)
{
	glActiveTexture(GL_TEXTURE0 + bindingPoint);
	glBindTexture(GL_TEXTURE_2D, texId);
	glUniform1i(id, bindingPoint);
}

void cm_enable_color_blend(void) { glEnable(GL_BLEND); }
void cm_disable_color_blend(void) { glDisable(GL_BLEND); }
void cm_enable_depth_test(void) { glEnable(GL_DEPTH_TEST); }
void cm_disable_depth_test(void) { glDisable(GL_DEPTH_TEST); }
void cm_enable_depth_mask(void) { glDepthMask(GL_TRUE); }
void cm_disable_depth_mask(void) { glDepthMask(GL_FALSE); }
void cm_enable_backface_culling(void) { glEnable(GL_CULL_FACE); }
void cm_disable_backface_culling(void) { glDisable(GL_CULL_FACE); }

// Set color mask active for screen read/draw
void cm_color_mask(uint32_t mask) { glColorMask((mask & CM_RED) > 0, (mask & CM_GREEN) > 0,
												 (mask & CM_BLUE) > 0, (mask & CM_ALPHA) > 0); }

void cm_set_cull_face(int mode)
{
	switch (mode)
	{
		case CM_CULL_FACE_BACK: glCullFace(GL_BACK); break;
		case CM_CULL_FACE_FRONT: glCullFace(GL_FRONT); break;
		default: break;
	}
}

void cm_enable_scissor_test(void) { glEnable(GL_SCISSOR_TEST); }
void cm_disable_scissor_test(void) { glDisable(GL_SCISSOR_TEST); }
void cm_scissor(int x, int y, int width, int height) { glScissor(x, y, width, height); }

// Enable wire mode
void cm_enable_wire_mode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void cm_enable_point_mode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glEnable(GL_PROGRAM_POINT_SIZE);
}
// Disable wire mode
void cm_disable_wire_mode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Set the line drawing width
void set_line_width(float width) { glLineWidth(width); }

// Get the line drawing width
float get_line_width(void)
{
	float width = 0;
	glGetFloatv(GL_LINE_WIDTH, &width);
	return width;
}

// Enable line aliasing
void enable_smooth_lines(void) { glEnable(GL_LINE_SMOOTH); }
// Disable line aliasing
void disable_smooth_lines(void) { glDisable(GL_LINE_SMOOTH); }