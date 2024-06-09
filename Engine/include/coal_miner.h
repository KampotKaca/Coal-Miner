#ifndef COAL_MINER_H
#define COAL_MINER_H

#define CM_MALLOC(sz) malloc(sz)
#define CM_CALLOC(n, sz) calloc(n, sz)
#define CM_REALLOC(ptr,sz) realloc(ptr, sz)
#define CM_FREE(ptr) free(ptr)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"
#include "coal_math.h"

//region Inputs
// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
typedef enum KeyboardKey
{
	KEY_NULL            = 0,        // Key: NULL, used for no key pressed
	// Alphanumeric keys
	KEY_APOSTROPHE      = 39,       // Key: '
	KEY_COMMA           = 44,       // Key: ,
	KEY_MINUS           = 45,       // Key: -
	KEY_PERIOD          = 46,       // Key: .
	KEY_SLASH           = 47,       // Key: /
	KEY_ZERO            = 48,       // Key: 0
	KEY_ONE             = 49,       // Key: 1
	KEY_TWO             = 50,       // Key: 2
	KEY_THREE           = 51,       // Key: 3
	KEY_FOUR            = 52,       // Key: 4
	KEY_FIVE            = 53,       // Key: 5
	KEY_SIX             = 54,       // Key: 6
	KEY_SEVEN           = 55,       // Key: 7
	KEY_EIGHT           = 56,       // Key: 8
	KEY_NINE            = 57,       // Key: 9
	KEY_SEMICOLON       = 59,       // Key: ;
	KEY_EQUAL           = 61,       // Key: =
	KEY_A               = 65,       // Key: A | a
	KEY_B               = 66,       // Key: B | b
	KEY_C               = 67,       // Key: C | c
	KEY_D               = 68,       // Key: D | d
	KEY_E               = 69,       // Key: E | e
	KEY_F               = 70,       // Key: F | f
	KEY_G               = 71,       // Key: G | g
	KEY_H               = 72,       // Key: H | h
	KEY_I               = 73,       // Key: I | i
	KEY_J               = 74,       // Key: J | j
	KEY_K               = 75,       // Key: K | k
	KEY_L               = 76,       // Key: L | l
	KEY_M               = 77,       // Key: M | m
	KEY_N               = 78,       // Key: N | n
	KEY_O               = 79,       // Key: O | o
	KEY_P               = 80,       // Key: P | p
	KEY_Q               = 81,       // Key: Q | q
	KEY_R               = 82,       // Key: R | r
	KEY_S               = 83,       // Key: S | s
	KEY_T               = 84,       // Key: T | t
	KEY_U               = 85,       // Key: U | u
	KEY_V               = 86,       // Key: V | v
	KEY_W               = 87,       // Key: W | w
	KEY_X               = 88,       // Key: X | x
	KEY_Y               = 89,       // Key: Y | y
	KEY_Z               = 90,       // Key: Z | z
	KEY_LEFT_BRACKET    = 91,       // Key: [
	KEY_BACKSLASH       = 92,       // Key: '\'
	KEY_RIGHT_BRACKET   = 93,       // Key: ]
	KEY_GRAVE           = 96,       // Key: `
	// Function keys
	KEY_SPACE           = 32,       // Key: Space
	KEY_ESCAPE          = 256,      // Key: Esc
	KEY_ENTER           = 257,      // Key: Enter
	KEY_TAB             = 258,      // Key: Tab
	KEY_BACKSPACE       = 259,      // Key: Backspace
	KEY_INSERT          = 260,      // Key: Ins
	KEY_DELETE          = 261,      // Key: Del
	KEY_RIGHT           = 262,      // Key: Cursor right
	KEY_LEFT            = 263,      // Key: Cursor left
	KEY_DOWN            = 264,      // Key: Cursor down
	KEY_UP              = 265,      // Key: Cursor up
	KEY_PAGE_UP         = 266,      // Key: Page up
	KEY_PAGE_DOWN       = 267,      // Key: Page down
	KEY_HOME            = 268,      // Key: Home
	KEY_END             = 269,      // Key: End
	KEY_CAPS_LOCK       = 280,      // Key: Caps lock
	KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
	KEY_NUM_LOCK        = 282,      // Key: Num lock
	KEY_PRINT_SCREEN    = 283,      // Key: Print screen
	KEY_PAUSE           = 284,      // Key: Pause
	KEY_F1              = 290,      // Key: F1
	KEY_F2              = 291,      // Key: F2
	KEY_F3              = 292,      // Key: F3
	KEY_F4              = 293,      // Key: F4
	KEY_F5              = 294,      // Key: F5
	KEY_F6              = 295,      // Key: F6
	KEY_F7              = 296,      // Key: F7
	KEY_F8              = 297,      // Key: F8
	KEY_F9              = 298,      // Key: F9
	KEY_F10             = 299,      // Key: F10
	KEY_F11             = 300,      // Key: F11
	KEY_F12             = 301,      // Key: F12
	KEY_LEFT_SHIFT      = 340,      // Key: Shift left
	KEY_LEFT_CONTROL    = 341,      // Key: Control left
	KEY_LEFT_ALT        = 342,      // Key: Alt left
	KEY_LEFT_SUPER      = 343,      // Key: Super left
	KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
	KEY_RIGHT_CONTROL   = 345,      // Key: Control right
	KEY_RIGHT_ALT       = 346,      // Key: Alt right
	KEY_RIGHT_SUPER     = 347,      // Key: Super right
	KEY_KB_MENU         = 348,      // Key: KB menu
	// Keypad keys
	KEY_KP_0            = 320,      // Key: Keypad 0
	KEY_KP_1            = 321,      // Key: Keypad 1
	KEY_KP_2            = 322,      // Key: Keypad 2
	KEY_KP_3            = 323,      // Key: Keypad 3
	KEY_KP_4            = 324,      // Key: Keypad 4
	KEY_KP_5            = 325,      // Key: Keypad 5
	KEY_KP_6            = 326,      // Key: Keypad 6
	KEY_KP_7            = 327,      // Key: Keypad 7
	KEY_KP_8            = 328,      // Key: Keypad 8
	KEY_KP_9            = 329,      // Key: Keypad 9
	KEY_KP_DECIMAL      = 330,      // Key: Keypad .
	KEY_KP_DIVIDE       = 331,      // Key: Keypad /
	KEY_KP_MULTIPLY     = 332,      // Key: Keypad *
	KEY_KP_SUBTRACT     = 333,      // Key: Keypad -
	KEY_KP_ADD          = 334,      // Key: Keypad +
	KEY_KP_ENTER        = 335,      // Key: Keypad Enter
	KEY_KP_EQUAL        = 336,      // Key: Keypad =
	// Android key buttons
	KEY_BACK            = 4,        // Key: Android back button
	KEY_MENU            = 82,       // Key: Android menu button
	KEY_VOLUME_UP       = 24,       // Key: Android volume up button
	KEY_VOLUME_DOWN     = 25        // Key: Android volume down button
} KeyboardKey;

// Add backwards compatibility support for deprecated names
#define MOUSE_LEFT_BUTTON   MOUSE_BUTTON_LEFT
#define MOUSE_RIGHT_BUTTON  MOUSE_BUTTON_RIGHT
#define MOUSE_MIDDLE_BUTTON MOUSE_BUTTON_MIDDLE

// Mouse buttons
typedef enum MouseButton
{
	MOUSE_BUTTON_LEFT    = 0,       // Mouse button left
	MOUSE_BUTTON_RIGHT   = 1,       // Mouse button right
	MOUSE_BUTTON_MIDDLE  = 2,       // Mouse button middle (pressed wheel)
	MOUSE_BUTTON_SIDE    = 3,       // Mouse button side (advanced mouse device)
	MOUSE_BUTTON_EXTRA   = 4,       // Mouse button extra (advanced mouse device)
	MOUSE_BUTTON_FORWARD = 5,       // Mouse button forward (advanced mouse device)
	MOUSE_BUTTON_BACK    = 6,       // Mouse button back (advanced mouse device)
} MouseButton;

// Mouse cursor
typedef enum MouseCursor
{
	MOUSE_CURSOR_DEFAULT       = 0,     // Default pointer shape
	MOUSE_CURSOR_ARROW         = 1,     // Arrow shape
	MOUSE_CURSOR_IBEAM         = 2,     // Text writing cursor shape
	MOUSE_CURSOR_CROSSHAIR     = 3,     // Cross shape
	MOUSE_CURSOR_POINTING_HAND = 4,     // Pointing hand cursor
	MOUSE_CURSOR_RESIZE_EW     = 5,     // Horizontal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NS     = 6,     // Vertical resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NWSE   = 7,     // Top-left to bottom-right diagonal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_NESW   = 8,     // The top-right to bottom-left diagonal resize/move arrow shape
	MOUSE_CURSOR_RESIZE_ALL    = 9,     // The omnidirectional resize/move cursor shape
	MOUSE_CURSOR_NOT_ALLOWED   = 10     // The operation-not-allowed shape
} MouseCursor;

// Gamepad buttons
typedef enum GamepadButton
{
	GAMEPAD_BUTTON_UNKNOWN = 0,         // Unknown button, just for error checking
	GAMEPAD_BUTTON_LEFT_FACE_UP,        // Gamepad left DPAD up button
	GAMEPAD_BUTTON_LEFT_FACE_RIGHT,     // Gamepad left DPAD right button
	GAMEPAD_BUTTON_LEFT_FACE_DOWN,      // Gamepad left DPAD down button
	GAMEPAD_BUTTON_LEFT_FACE_LEFT,      // Gamepad left DPAD left button
	GAMEPAD_BUTTON_RIGHT_FACE_UP,       // Gamepad right button up (i.e. PS3: Triangle, Xbox: Y)
	GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,    // Gamepad right button right (i.e. PS3: Square, Xbox: X)
	GAMEPAD_BUTTON_RIGHT_FACE_DOWN,     // Gamepad right button down (i.e. PS3: Cross, Xbox: A)
	GAMEPAD_BUTTON_RIGHT_FACE_LEFT,     // Gamepad right button left (i.e. PS3: Circle, Xbox: B)
	GAMEPAD_BUTTON_LEFT_TRIGGER_1,      // Gamepad top/back trigger left (first), it could be a trailing button
	GAMEPAD_BUTTON_LEFT_TRIGGER_2,      // Gamepad top/back trigger left (second), it could be a trailing button
	GAMEPAD_BUTTON_RIGHT_TRIGGER_1,     // Gamepad top/back trigger right (one), it could be a trailing button
	GAMEPAD_BUTTON_RIGHT_TRIGGER_2,     // Gamepad top/back trigger right (second), it could be a trailing button
	GAMEPAD_BUTTON_MIDDLE_LEFT,         // Gamepad center buttons, left one (i.e. PS3: Select)
	GAMEPAD_BUTTON_MIDDLE,              // Gamepad center buttons, middle one (i.e. PS3: PS, Xbox: XBOX)
	GAMEPAD_BUTTON_MIDDLE_RIGHT,        // Gamepad center buttons, right one (i.e. PS3: Start)
	GAMEPAD_BUTTON_LEFT_THUMB,          // Gamepad joystick pressed button left
	GAMEPAD_BUTTON_RIGHT_THUMB          // Gamepad joystick pressed button right
} GamepadButton;

// Gamepad axis
typedef enum GamepadAxis
{
	GAMEPAD_AXIS_LEFT_X        = 0,     // Gamepad left stick X axis
	GAMEPAD_AXIS_LEFT_Y        = 1,     // Gamepad left stick Y axis
	GAMEPAD_AXIS_RIGHT_X       = 2,     // Gamepad right stick X axis
	GAMEPAD_AXIS_RIGHT_Y       = 3,     // Gamepad right stick Y axis
	GAMEPAD_AXIS_LEFT_TRIGGER  = 4,     // Gamepad back trigger left, pressure level: [1..-1]
	GAMEPAD_AXIS_RIGHT_TRIGGER = 5      // Gamepad back trigger right, pressure level: [1..-1]
} GamepadAxis;
//endregion

//region Important Structures
// Pixel formats
// NOTE: Support depends on OpenGL version and platform
typedef enum {
	PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1, // 8 bit per pixel (no alpha)
	PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
	PIXELFORMAT_UNCOMPRESSED_R5G6B5,        // 16 bpp
	PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
	PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,      // 16 bpp (1 bit alpha)
	PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,      // 16 bpp (4 bit alpha)
	PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
	PIXELFORMAT_UNCOMPRESSED_R32,           // 32 bpp (1 channel - float)
	PIXELFORMAT_UNCOMPRESSED_R32G32B32,     // 32*3 bpp (3 channels - float)
	PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,  // 32*4 bpp (4 channels - float)
	PIXELFORMAT_UNCOMPRESSED_R16,           // 16 bpp (1 channel - half float)
	PIXELFORMAT_UNCOMPRESSED_R16G16B16,     // 16*3 bpp (3 channels - half float)
	PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,  // 16*4 bpp (4 channels - half float)
	PIXELFORMAT_COMPRESSED_DXT1_RGB,        // 4 bpp (no alpha)
	PIXELFORMAT_COMPRESSED_DXT1_RGBA,       // 4 bpp (1 bit alpha)
	PIXELFORMAT_COMPRESSED_DXT3_RGBA,       // 8 bpp
	PIXELFORMAT_COMPRESSED_DXT5_RGBA,       // 8 bpp
	PIXELFORMAT_COMPRESSED_ETC1_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_ETC2_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,   // 8 bpp
	PIXELFORMAT_COMPRESSED_PVRT_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_PVRT_RGBA,       // 4 bpp
	PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,   // 8 bpp
	PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA    // 2 bpp
} PixelFormat;

// Texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
typedef enum {
	TEXTURE_FILTER_POINT = 0,               // No filter, just pixel approximation
	TEXTURE_FILTER_BILINEAR,                // Linear filtering
	TEXTURE_FILTER_TRILINEAR,               // Trilinear filtering (linear with mipmaps)
	TEXTURE_FILTER_ANISOTROPIC_4X,          // Anisotropic filtering 4x
	TEXTURE_FILTER_ANISOTROPIC_8X,          // Anisotropic filtering 8x
	TEXTURE_FILTER_ANISOTROPIC_16X,         // Anisotropic filtering 16x
} TextureFilter;

// Texture parameters: wrap mode
typedef enum {
	TEXTURE_WRAP_REPEAT = 0,                // Repeats texture in tiled mode
	TEXTURE_WRAP_CLAMP,                     // Clamps texture to edge pixel in tiled mode
	TEXTURE_WRAP_MIRROR_REPEAT,             // Mirrors and repeats the texture in tiled mode
	TEXTURE_WRAP_MIRROR_CLAMP               // Mirrors and clamps to border the texture in tiled mode
} TextureWrap;

// Rectangle, 4 components
typedef struct Rect
{
	V2 center;
	V2 size;
} Rect;

// Image, pixel data stored in CPU memory (RAM)
typedef struct Image
{
	void *data;             // Image raw data
	int width;              // Image base width
	int height;             // Image base height
	int mipmaps;            // Mipmap levels, 1 by default
	int format;             // Data format (PixelFormat type)
} Image;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct Texture
{
	unsigned int id;        // OpenGL texture id
	int width;              // Texture base width
	int height;             // Texture base height
	int mipmaps;            // Mipmap levels, 1 by default
	int format;             // Data format (PixelFormat type)
} Texture;

// RenderTexture, fbo for texture rendering
typedef struct RenderTexture
{
	unsigned int id;        // OpenGL framebuffer object id
	Texture texture;        // Color buffer attachment texture
	Texture depth;          // Depth buffer attachment texture
} RenderTexture;

// Shader
typedef struct Shader
{
	unsigned int id;        // Shader program id
	int *locs;              // Shader locations array (MAX_SHADER_LOCATIONS)
} Shader;

// Transform, vertex transformation data
typedef struct Transform
{
	V3 translation;         // Translation
	Quaternion rotation;    // Rotation
	V3 scale;               // Scale
} Transform;

typedef struct Mesh
{
	unsigned int vertexCount;        // Number of vertices stored in arrays
	unsigned int triangleCount;      // Number of triangles stored (indexed or not)
	
	// Vertex attributes data
	float *vertices;        // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
	float *texcoords;       // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
	float *texcoords2;      // Vertex texture second coordinates (UV - 2 components per vertex) (shader-location = 5)
	float *normals;         // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
	float *tangents;        // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
	unsigned char *colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
	unsigned short *indices;    // Vertex indices (in case vertex data comes indexed)
	
	// Animation vertex data
	float *animVertices;    // Animated vertex positions (after bones transformations)
	float *animNormals;     // Animated normals (after bones transformations)
	unsigned char *boneIds; // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning)
	float *boneWeights;     // Vertex bone weight, up to 4 bones influence by vertex (skinning)
} Mesh;

// Bone, skeletal animation bone
typedef struct BoneInfo
{
	char name[32];          // Bone name
	int parent;             // Bone parent
} BoneInfo;

// Model, meshes, materials and animation data
typedef struct Model
{
	M4x4 transform;       // Local transform matrix
	
	unsigned int meshCount;          // Number of meshes
	Mesh *meshes;           // Meshes array
	
	// Animation data
	unsigned int boneCount;          // Number of bones
	BoneInfo *bones;        // Bones information (skeleton)
	Transform *bindPose;    // Bones base transformation (pose)
} Model;

// Camera, defines position/orientation in 3d space
typedef struct Camera3D
{
	V3 position;       // Camera position
	V3 target;         // Camera target it looks-at
	V3 up;             // Camera up vector (rotation over its axis)
	float fov;         // Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic
	int projection;    // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
	float nearPlane;
	float farPlane;
} Camera3D;

typedef struct Vbo
{
	unsigned int id;
	unsigned int dataSize;
	bool isStatic;
	void* data;
}Vbo;

typedef struct VaoAttribute
{
	unsigned int size;
	unsigned int type;
	bool normalized;
	unsigned int stride;
}VaoAttribute;

typedef struct Vao
{
	unsigned int id;
	VaoAttribute* attributes;
	unsigned int attributeCount;
	Vbo* vbos;
	unsigned int vboCount;
}Vao;

// Camera projection
typedef enum CameraProjection
{
	CAMERA_PERSPECTIVE = 0,         // Perspective projection
	CAMERA_ORTHOGRAPHIC             // Orthographic projection
} CameraProjection;

typedef enum VaoDataType
{
	CM_BYTE = 0x1400,
	CM_UBYTE = 0x1401,
	CM_SHORT = 0x1402,
	CM_USHORT = 0x1403,
	CM_INT = 0x1404,
	CM_UINT = 0x1405,
	CM_HALF_FLOAT = 0x140B,
	CM_FLOAT = 0x1406,
} VaoDataType;

//endregion

extern Image cm_load_image(const char* filePath);
extern void cm_unload_image(Image image);
extern void cm_unload_images(Image* image, int size);
extern Image cm_load_image_raw(const char *filePath, int width, int height, int format, int headerSize);
extern Image cm_load_image_svg(const char *fileNameOrString, int width, int height);
extern Texture cm_load_texture(const char* filePath);
extern Texture cm_load_texture_from_image(Image image);
extern Shader cm_load_shader(const char *vsPath, const char *fsPath);
extern Shader cm_load_shader_from_memory(const char *vsCode, const char *fsCode);
extern void cm_unload_shader(Shader shader);

extern void cm_begin_shader_mode(Shader shader);
extern void cm_end_shader_mode();

extern void cm_begin_mode_3d(Camera3D camera);
extern void cm_end_mode_3d();

extern bool cm_load_ubo(unsigned int blockId, unsigned int dataSize, void* data);
extern Vao cm_load_vao(VaoAttribute* attributes, unsigned int attributeCount, Vbo* vbos, unsigned int vboCount);
extern void cm_unload_vao(Vao vao);
extern Vbo cm_load_vbo(unsigned int dataSize, void* data, bool isStatic);
extern void cm_unload_vbo(Vbo vbo);

extern void cm_draw_vao(Vao vao);

extern const char *cm_get_file_extension(const char *filePath);
char *cm_load_file_text(const char *filePath);
void cm_unload_file_text(char *text);

#endif //COAL_MINER_H
