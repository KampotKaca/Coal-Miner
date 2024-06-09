#ifndef COAL_MATH_H
#define COAL_MATH_H

#define PI 3.14159265358979323846f
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

typedef struct M4x4
{
	float m0, m4, m8, m12;  // M4x4 first row (4 components)
	float m1, m5, m9, m13;  // M4x4 second row (4 components)
	float m2, m6, m10, m14; // M4x4 third row (4 components)
	float m3, m7, m11, m15; // M4x4 fourth row (4 components)
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

typedef struct float3
{
	float v[3];
} float3;

typedef struct float16
{
	float v[16];
} float16;
//endregion

typedef V4 Quaternion;

//region V2

extern V2 v2_zero(void);
extern V2 v2_one(void);
extern V2 v2_add(V2 v1, V2 v2);
extern V2 v2_add_f(V2 v, float add);
extern V2 v2_sub(V2 v, V2 v2);
extern V2 v2_sub_f(V2 v, float sub);
extern float v2_length(V2 v);
extern float v2_length_sqr(V2 v);
extern float v2_dot(V2 v1, V2 v2);
extern float v2_distance(V2 v1, V2 v2);
extern float v2_distance_sqr(V2 v1, V2 v2);
extern float v2_angle(V2 v1, V2 v2);
extern float v2_line_angle(V2 start, V2 end);
extern V2 v2_scale(V2 v, float scale);
extern V2 v2_mult(V2 v1, V2 v2);
extern V2 v2_negate(V2 v);
extern V2 v2_div(V2 v1, V2 v2);
extern V2 v2_norm(V2 v);
extern V2 v2_transform(V2 v, M4x4 mat);
extern V2 v2_lerp(V2 v1, V2 v2, float amount);
extern V2 v2_reflect(V2 v, V2 normal);
extern V2 v2_min(V2 v1, V2 v2);
extern V2 v2_max(V2 v1, V2 v2);
extern V2 v2_rotate(V2 v, float angle);
extern V2 v2_move_towards(V2 v, V2 target, float maxDistance);
extern V2 v2_invert(V2 v);
extern V2 v2_clamp(V2 v, V2 min, V2 max);
extern V2 v2_clamp_f(V2 v, float min, float max);
extern int v2_equals(V2 p, V2 q);
extern V2 v2_refract(V2 v, V2 n, float r);

//endregion

//region V3

extern V3 v3_zero(void);
extern V3 v3_one(void);
extern V3 v3_add(V3 v1, V3 v2);
extern V3 v3_add_f(V3 v, float add);
extern V3 v3_sub(V3 v1, V3 v2);
extern V3 v3_sub_f(V3 v, float sub);
extern V3 v3_scale(V3 v, float scalar);
extern V3 v3_mult(V3 v1, V3 v2);
extern V3 v3_cross(V3 v1, V3 v2);
extern V3 v3_perp(V3 v);
extern float v3_length(V3 v);
extern float v3_length_sqr(V3 v);
extern float v3_dot(V3 v1, V3 v2);
extern float v3_distance(V3 v1, V3 v2);
extern float v3_distance_sqr(V3 v1, V3 v2);
extern float v3_angle(V3 v1, V3 v2);
extern V3 v3_negate(V3 v);
extern V3 v3_div(V3 v1, V3 v2);
extern V3 v3_norm(V3 v);
extern V3 v3_project(V3 v1, V3 v2);
extern V3 v3_reject(V3 v1, V3 v2);
extern void v3_ortho_norm(V3 *v1, V3 *v2);
extern V3 v3_transform(V3 v, M4x4 mat);
extern V3 v3_rotate_by_quaternion(V3 v, Quaternion q);
extern V3 v3_rotate_by_axis_angle(V3 v, V3 axis, float angle);
extern V3 v3_move_towards(V3 v, V3 target, float maxDistance);
extern V3 v3_lerp(V3 v1, V3 v2, float amount);
extern V3 v3_cubic_hermite(V3 v1, V3 tangent1, V3 v2, V3 tangent2, float amount);
extern V3 v3_reflect(V3 v, V3 normal);
extern V3 v3_min(V3 v1, V3 v2);
extern V3 v3_max(V3 v1, V3 v2);
extern V3 v3_barycenter(V3 p, V3 a, V3 b, V3 c);
extern V3 v3_unproject(V3 source, M4x4 projection, M4x4 view);
extern float3 v3_to_float3(V3 v);
extern V3 v3_invert(V3 v);
extern V3 v3_clamp(V3 v, V3 min, V3 max);
extern V3 v3_clamp_f(V3 v, float min, float max);
extern int v3_equals(V3 p, V3 q);
extern V3 v3_refract(V3 v, V3 n, float r);

//endregion

//region M4x4

extern M4x4 m4x4_scale(float x, float y, float z);
extern M4x4 m4x4_scale_V3(V3 v3);
extern M4x4 m4x4_identity(void);

extern M4x4 m4x4_frustum(double left, double right, double bottom, double top, double near, double far);
extern M4x4 m4x4_perspective(double fov, double aspect, double nearPlane, double farPlane);
extern M4x4 m4x4_orthographic(double left, double right, double bottom, double top, double nearPlane, double farPlane);
extern M4x4 m4x4_look_at(V3 eye, V3 target, V3 up);
extern float16 m4x4_to_float16(M4x4 mat);

extern float m4x4_determinant(M4x4 mat);
extern float m4x4_trace(M4x4 mat);
extern M4x4 m4x4_transpose(M4x4 mat);
extern M4x4 m4x4_invert(M4x4 mat);
extern M4x4 m4x4_add(M4x4 left, M4x4 right);
extern M4x4 m4x4_sub(M4x4 left, M4x4 right);
extern M4x4 m4x4_mult(M4x4 left, M4x4 right);
extern M4x4 m4x4_translate(float x, float y, float z);
extern M4x4 m4x4_rotate(V3 axis, float angle);
extern M4x4 m4x4_rotate_x(float angle);
extern M4x4 m4x4_rotate_y(float angle);
extern M4x4 m4x4_rotate_z(float angle);
extern M4x4 m4x4_rotate_xyz(V3 angle);
extern M4x4 m4x4_rotate_zyx(V3 angle);

//endregion

//region Quaternion
extern Quaternion quaternion_add(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_add_f(Quaternion q, float add);
extern Quaternion quaternion_sub(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_sub_f(Quaternion q, float sub);
extern Quaternion quaternion_identity(void);
extern float quaternion_length(Quaternion q);
extern Quaternion quaternion_norm(Quaternion q);
extern Quaternion quaternion_invert(Quaternion q);
extern Quaternion quaternion_mult(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_scale(Quaternion q, float mul);
extern Quaternion quaternion_div(Quaternion q1, Quaternion q2);
extern Quaternion quaternion_lerp(Quaternion q1, Quaternion q2, float amount);
extern Quaternion quaternion_nlerp(Quaternion q1, Quaternion q2, float amount);
extern Quaternion quaternion_slerp(Quaternion q1, Quaternion q2, float amount);
extern Quaternion quaternion_cubic_hermite_spline(Quaternion q1, Quaternion outTangent1, Quaternion q2, Quaternion inTangent2, float t);
extern Quaternion quaternion_from_v3_to_v3(V3 from, V3 to);
extern Quaternion quaternion_from_m4x4(M4x4 mat);
extern M4x4 quaternion_to_m4x4(Quaternion q);
extern Quaternion quaternion_from_angle_axis(V3 axis, float angle);
extern void quaternion_to_angle_axis(Quaternion q, V3 *outAxis, float *outAngle);
extern Quaternion quaternion_from_euler(V3 euler);
extern V3 quaternion_to_euler(Quaternion q);
extern Quaternion quaternion_transform(Quaternion q, M4x4 mat);
extern int quaternion_equals(Quaternion p, Quaternion q);
//endregion

#endif //COAL_MATH_H
