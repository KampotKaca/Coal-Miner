#include "coal_image.h"
#include <stb_image.h>
#include "cmgl.h"

Image cm_load_image(const char* filePath)
{
	Image image = { 0 };

#if defined(SUPPORT_FILEFORMAT_PNG) || \
    defined(SUPPORT_FILEFORMAT_BMP) || \
    defined(SUPPORT_FILEFORMAT_TGA) || \
    defined(SUPPORT_FILEFORMAT_JPG) || \
    defined(SUPPORT_FILEFORMAT_GIF) || \
    defined(SUPPORT_FILEFORMAT_PIC) || \
    defined(SUPPORT_FILEFORMAT_HDR) || \
    defined(SUPPORT_FILEFORMAT_PNM) || \
    defined(SUPPORT_FILEFORMAT_PSD)

#define STBI_REQUIRED
#endif
	
	// Loading file to memory
	int dataSize = 0;
	unsigned char *fileData = cm_load_file_data(filePath, &dataSize);
	
	// Loading image from memory data
	if (fileData != NULL) image = cm_load_image_from_memory(cm_get_file_extension(filePath), fileData, dataSize);
	
	CM_FREE(fileData);
	
	return image;
}

void cm_unload_image(Image image)
{
	CM_FREE(image.data);
}

void cm_unload_images(Image* image, int size)
{
	for (int i = 0; i < size; ++i) cm_unload_image(image[i]);
}

Texture cm_load_texture(const char* filePath, TextureFlags wrap, TextureFlags filter)
{
	Texture texture = { 0 };
	Image image = cm_load_image(filePath);
	
	if (image.data != NULL)
	{
		texture = cm_load_texture_from_image(image, wrap, filter);
		cm_unload_image(image);
	}
	
	return texture;
}

Texture cm_load_texture_from_image(Image image, TextureFlags wrap, TextureFlags filter)
{
	Texture texture = { 0 };
	
	if ((image.width != 0) && (image.height != 0))
	{
		texture.id = load_texture(image.data, image.width, image.height, wrap, filter, image.format, image.mipmaps);
	}
	else printf("IMAGE: Data is not valid to load texture");
	
	texture.width = image.width;
	texture.height = image.height;
	texture.mipmaps = image.mipmaps;
	texture.format = image.format;
	
	return texture;
}

unsigned char* cm_load_file_data(const char* filePath, int* dataSize)
{
	unsigned char *data = NULL;
	*dataSize = 0;
	
	if (filePath != NULL)
	{
		FILE* file;

        if (!fopen_s(&file, filePath, "rb"))
        {
            // WARNING: On binary streams SEEK_END could not be found,
            // using fseek() and ftell() could not work in some (rare) cases
            fseek(file, 0, SEEK_END);
            int size = ftell(file);     // WARNING: ftell() returns 'long int', maximum size returned is INT_MAX (2147483647 bytes)
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                data = (unsigned char*)CM_MALLOC(size * sizeof(unsigned char));

                if (data != NULL)
                {
                    // NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
                    size_t count = fread(data, sizeof(unsigned char), size, file);
                    
                    // WARNING: fread() returns a size_t value, usually 'unsigned int' (32bit compilation) and 'unsigned long long' (64bit compilation)
                    // dataSize is unified along raylib as a 'int' type, so, for file-sizes > INT_MAX (2147483647 bytes) we have a limitation
                    if (count > 2147483647)
                    {
                        printf("FILEIO: [%s] File is bigger than 2147483647 bytes, avoid using LoadFileData()", filePath);
                        
                        CM_FREE(data);
                        data = NULL;
                    }
                    else
                    {
                        *dataSize = (int)count;

                        if ((*dataSize) != size) printf("FILEIO: [%s] File partially loaded (%i bytes out of %i)", filePath, *dataSize, (int)count);
                    }
                }
                else printf("FILEIO: [%s] Failed to allocated memory for file reading", filePath);
            }
            else printf("FILEIO: [%s] Failed to read file", filePath);

            fclose(file);
        }
        else printf("FILEIO: [%s] Failed to open file", filePath);
	}
	else printf("FILEIO: File name provided is not valid");
	
	return data;
}

Image cm_load_image_from_memory(const char *fileType, const unsigned char *fileData, int dataSize)
{
	Image image = { 0 };
	
	if ((false)
#if defined(SUPPORT_FILEFORMAT_PNG)
		|| (strcmp(fileType, ".png") == 0) || (strcmp(fileType, ".PNG") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_BMP)
		|| (strcmp(fileType, ".bmp") == 0) || (strcmp(fileType, ".BMP") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_TGA)
		|| (strcmp(fileType, ".tga") == 0) || (strcmp(fileType, ".TGA") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_JPG)
		|| (strcmp(fileType, ".jpg") == 0) || (strcmp(fileType, ".jpeg") == 0)
        || (strcmp(fileType, ".JPG") == 0) || (strcmp(fileType, ".JPEG") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_PIC)
		|| (strcmp(fileType, ".pic") == 0) || (strcmp(fileType, ".PIC") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_PNM)
		|| (strcmp(fileType, ".ppm") == 0) || (strcmp(fileType, ".pgm") == 0)
        || (strcmp(fileType, ".PPM") == 0) || (strcmp(fileType, ".PGM") == 0)
#endif
#if defined(SUPPORT_FILEFORMAT_PSD)
		|| (strcmp(fileType, ".psd") == 0) || (strcmp(fileType, ".PSD") == 0)
#endif
			)
	{
#if defined(STBI_REQUIRED)
		// NOTE: Using stb_image to load images (Supports multiple image formats)

        if (fileData != NULL)
        {
            int comp = 0;
            image.data = stbi_load_from_memory(fileData, dataSize, &image.width, &image.height, &comp, 0);
			
            if (image.data != NULL)
            {
                image.mipmaps = 1;

                if (comp == 1) image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
                else if (comp == 2) image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
                else if (comp == 3) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
                else if (comp == 4) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            }
        }
#endif
	}
#if defined(SUPPORT_FILEFORMAT_HDR)
		else if ((strcmp(fileType, ".hdr") == 0) || (strcmp(fileType, ".HDR") == 0))
    {
#if defined(STBI_REQUIRED)
        if (fileData != NULL)
        {
            int comp = 0;
            image.data = stbi_loadf_from_memory(fileData, dataSize, &image.width, &image.height, &comp, 0);

            image.mipmaps = 1;

            if (comp == 1) image.format = PIXELFORMAT_UNCOMPRESSED_R32;
            else if (comp == 3) image.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32;
            else if (comp == 4) image.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
            else
            {
                TRACELOG(LOG_WARNING, "IMAGE: HDR file format not supported");
                UnloadImage(image);
            }
        }
#endif
    }
#endif
#if defined(SUPPORT_FILEFORMAT_QOI)
		else if ((strcmp(fileType, ".qoi") == 0) || (strcmp(fileType, ".QOI") == 0))
    {
        if (fileData != NULL)
        {
            qoi_desc desc = { 0 };
            image.data = qoi_decode(fileData, dataSize, &desc, 4);
            image.width = desc.width;
            image.height = desc.height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
        }
    }
#endif
#if defined(SUPPORT_FILEFORMAT_SVG)
		else if ((strcmp(fileType, ".svg") == 0) || (strcmp(fileType, ".SVG") == 0))
    {
        // Validate fileData as valid SVG string data
        //<svg xmlns="http://www.w3.org/2000/svg" width="2500" height="2484" viewBox="0 0 192.756 191.488">
        if ((fileData != NULL) &&
            (fileData[0] == '<') &&
            (fileData[1] == 's') &&
            (fileData[2] == 'v') &&
            (fileData[3] == 'g'))
        {
            struct NSVGimage *svgImage = nsvgParse(fileData, "px", 96.0f);
            unsigned char *img = RL_MALLOC(svgImage->width*svgImage->height*4);

            // Rasterize
            struct NSVGrasterizer *rast = nsvgCreateRasterizer();
            nsvgRasterize(rast, svgImage, 0, 0, 1.0f, img, svgImage->width, svgImage->height, svgImage->width*4);

            // Populate image struct with all data
            image.data = img;
            image.width = svgImage->width;
            image.height = svgImage->height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

            nsvgDelete(svgImage);
            nsvgDeleteRasterizer(rast);
        }
    }
#endif
#if defined(SUPPORT_FILEFORMAT_PKM)
		else if ((strcmp(fileType, ".pkm") == 0) || (strcmp(fileType, ".PKM") == 0))
    {
        image.data = rl_load_pkm_from_memory(fileData, dataSize, &image.width, &image.height, &image.format, &image.mipmaps);
    }
#endif
#if defined(SUPPORT_FILEFORMAT_KTX)
		else if ((strcmp(fileType, ".ktx") == 0) || (strcmp(fileType, ".KTX") == 0))
    {
        image.data = rl_load_ktx_from_memory(fileData, dataSize, &image.width, &image.height, &image.format, &image.mipmaps);
    }
#endif
#if defined(SUPPORT_FILEFORMAT_PVR)
		else if ((strcmp(fileType, ".pvr") == 0) || (strcmp(fileType, ".PVR") == 0))
    {
        image.data = rl_load_pvr_from_memory(fileData, dataSize, &image.width, &image.height, &image.format, &image.mipmaps);
    }
#endif
#if defined(SUPPORT_FILEFORMAT_ASTC)
		else if ((strcmp(fileType, ".astc") == 0) || (strcmp(fileType, ".ASTC") == 0))
    {
        image.data = rl_load_astc_from_memory(fileData, dataSize, &image.width, &image.height, &image.format, &image.mipmaps);
    }
#endif
	else printf("IMAGE: Data format not supported");
	
	if (image.data == NULL) printf("IMAGE: Failed to load image data");
	
	return image;
}