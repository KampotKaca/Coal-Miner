#ifndef COAL_MATH_H
#define COAL_MATH_H

#include "coal_miner.h"

extern M4x4 matrix_scale(float x, float y, float z)
{
	M4x4 result = { x, 0.0f, 0.0f, 0.0f,
	                  0.0f, y, 0.0f, 0.0f,
	                  0.0f, 0.0f, z, 0.0f,
	                  0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

extern M4x4 matrix_scale_V3(V3 v3)
{
	M4x4 result = { v3.x, 0.0f, 0.0f, 0.0f,
	                0.0f, v3.y, 0.0f, 0.0f,
	                0.0f, 0.0f, v3.z, 0.0f,
	                0.0f, 0.0f, 0.0f, 1.0f };

	return result;
}

#endif //COAL_MATH_H
