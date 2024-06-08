#ifndef COAL_MATH_H
#define COAL_MATH_H

typedef struct M4x4
{
	float m0, m4, m8, m12;  // Matrix first row (4 components)
	float m1, m5, m9, m13;  // Matrix second row (4 components)
	float m2, m6, m10, m14; // Matrix third row (4 components)
	float m3, m7, m11, m15; // Matrix fourth row (4 components)
} M4x4;

//region Vectors
typedef struct V2
{
	float x, y;
}V2;

typedef struct V2i
{
	int x, y;
}V2i;

typedef struct V3
{
	float x, y, z;
}V3;

typedef struct V3i
{
	int x, y, z;
}V3i;

typedef struct V4
{
	float x, y, z, w;
}V4;

typedef struct V4i
{
	int x, y, z, w;
}V4i;
//endregion

typedef V4 Quaternion;

extern M4x4 matrix_scale(float x, float y, float z);
extern M4x4 matrix_scale_V3(V3 v3);
extern M4x4 matrix_identity(void);

#endif //COAL_MATH_H
