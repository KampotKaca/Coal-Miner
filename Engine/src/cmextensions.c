#include "coal_miner.h"

void cm_print_mat4(vec4* mat)
{
	log_trace("00: %f, 10: %f, 20: %f, 30: %f", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
	log_trace("01: %f, 11: %f, 21: %f, 31: %f", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
	log_trace("02: %f, 12: %f, 22: %f, 32: %f", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
	log_trace("03: %f, 13: %f, 23: %f, 33: %f", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

void cm_print_v3(float* v3)
{
	log_info("x: %f, y: %f, z: %f\n", v3[0], v3[1], v3[2]);
}
void cm_print_quat(float* q)
{
	log_info("x: %f, y: %f, z: %f, w: %f\n", q[0], q[1], q[2], q[3]);
}

void cm_yp_to_direction(float yaw, float pitch, float* direction)
{
	glm_vec3(direction, GLM_VEC3_ZERO);
	float yawRad = glm_rad(yaw);
	float pitchRad = glm_rad(fmodf(pitch - 90, 180));
	
	direction[0] = cosf(pitchRad) * sinf(yawRad);
	direction[1] = sinf(pitchRad);
	direction[2] = cosf(pitchRad) * cosf(yawRad);
}

void cm_get_transformation(Transform* trs, vec4* m4)
{
	glm_mat4_identity(m4);
	glm_translate(m4, trs->position);
	glm_quat_rotate(m4, trs->rotation, m4);
	glm_scale(m4, trs->scale);
}

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
            uint32_t size = (uint32_t)ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                text = (char *)CM_MALLOC((size + 1)*sizeof(char));

                if (text != NULL)
                {
	                uint32_t count = (uint32_t)fread(text, sizeof(char), size, file);

                    // WARNING: \r\n is converted to \n on reading, so,
                    // read bytes count gets reduced by the number of lines
                    if (count < size)
                    {
						void* newMemory = CM_REALLOC(text, count + 1);
						if(newMemory == NULL)
						{
							perror("Unable to reallocate memory!!! exiting the program.\n");
							exit(-1);
						}
						else text = newMemory;
					}

                    // Zero-terminate the string
                    text[count] = '\0';

                }
                else log_error("FILEIO: [%s] Failed to allocated memory for file reading", filePath);
            }
            else log_error("FILEIO: [%s] Failed to read text file", filePath);

            fclose(file);
        }
        else log_error("FILEIO: [%s] Failed to open text file", filePath);
	}
	else log_error("FILEIO: File name provided is not valid");

	return text;
}

void cm_unload_file_text(char *text)
{
	CM_FREE(text);
}