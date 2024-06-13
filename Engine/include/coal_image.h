#ifndef COAL_IMAGE_H
#define COAL_IMAGE_H

#define STBI_REQUIRED
#define SUPPORT_FILEFORMAT_PNG
#define SUPPORT_FILEFORMAT_BMP
#define SUPPORT_FILEFORMAT_TGA
#define SUPPORT_FILEFORMAT_JPG
#define SUPPORT_FILEFORMAT_PIC
#define SUPPORT_FILEFORMAT_PNM
#define SUPPORT_FILEFORMAT_PSD

#include "coal_miner.h"

typedef enum rlPixelFormat
{
	RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,        // 8*2 bpp (2 channels)
	RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5,            // 16 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8,            // 24 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,          // 16 bpp (1 bit alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,          // 16 bpp (4 bit alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,          // 32 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R32,               // 32 bpp (1 channel - float)
	RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32,         // 32*3 bpp (3 channels - float)
	RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,      // 32*4 bpp (4 channels - float)
	RL_PIXELFORMAT_UNCOMPRESSED_R16,               // 16 bpp (1 channel - half float)
	RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16,         // 16*3 bpp (3 channels - half float)
	RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,      // 16*4 bpp (4 channels - half float)
	RL_PIXELFORMAT_COMPRESSED_DXT1_RGB,            // 4 bpp (no alpha)
	RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA,           // 4 bpp (1 bit alpha)
	RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA,           // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA,           // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC1_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC2_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,       // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_PVRT_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA,           // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,       // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA        // 2 bpp
} rlPixelFormat;

extern unsigned char* cm_load_file_data(const char* filePath, int* dataSize);
extern Image cm_load_image_from_memory(const char *fileType, const unsigned char *fileData, int dataSize);

#endif //COAL_IMAGE_H
