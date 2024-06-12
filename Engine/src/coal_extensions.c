#include "coal_miner.h"

const char *cm_get_file_extension(const char *filePath)
{
	const char *dot = strrchr(filePath, '.');

	if (!dot || dot == filePath) return NULL;

	return dot;
}

char *cm_load_file_text(const char *filePath)
{
	char *text = NULL;

	if (filePath != NULL)
	{
		FILE *file;

        if (!fopen_s(&file, filePath, "rt"))
        {
            // WARNING: When reading a file as 'text' file,
            // text mode causes carriage return-linefeed translation...
            // ...but using fseek() should return correct byte-offset
            fseek(file, 0, SEEK_END);
            unsigned int size = (unsigned int)ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                text = (char *)CM_MALLOC((size + 1)*sizeof(char));

                if (text != NULL)
                {
                    unsigned int count = (unsigned int)fread(text, sizeof(char), size, file);

                    // WARNING: \r\n is converted to \n on reading, so,
                    // read bytes count gets reduced by the number of lines
                    if (count < size) text = CM_REALLOC(text, count + 1);

                    // Zero-terminate the string
                    text[count] = '\0';

                }
                else printf("FILEIO: [%s] Failed to allocated memory for file reading", filePath);
            }
            else printf("FILEIO: [%s] Failed to read text file", filePath);

            fclose(file);
        }
        else printf("FILEIO: [%s] Failed to open text file", filePath);
	}
	else printf("FILEIO: File name provided is not valid");

	return text;
}

void cm_unload_file_text(char *text)
{
	CM_FREE(text);
}