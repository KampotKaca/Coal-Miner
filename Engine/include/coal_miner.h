#ifndef COAL_MINER_H
#define COAL_MINER_H

#define CM_MALLOC(sz) malloc(sz)
#define CM_CALLOC(n, sz) calloc(n, sz)
#define CM_REALLOC(ptr,sz) realloc(ptr, sz)
#define CM_FREE(ptr) free(ptr)

#define TO_RES_PATH(x, y) RES_PATH; strcat_s(x, MAX_PATH_SIZE, y)

#define TRANSFORM_INIT { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }
#define CAMERA_INIT { 0, 2, -5, 0, 0, 1, 0, 1, 0, 45, 0, 0.01, 200 }
#define GLOBAL_LIGHT_INIT { 0, -1, 0, 1, 1, 1, 1 }

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <dirent/dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include "coal_config.h"
#include <cglm/cglm.h>
#include <cglm/call.h>
#include <cglm/quat.h>
#include <inttypes.h>
#include "threadpool/cm_threadpool.h"
#include "list/list.h"
#include "log.h"

typedef char Path[MAX_PATH_SIZE];

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
	vec2 center;
	vec2 size;
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

typedef enum TextureFlags
{
	CM_TEXTURE_WRAP_REPEAT = 0x2901,
	CM_TEXTURE_WRAP_CLAMP_TO_EDGE = 0x812F,
	CM_TEXTURE_WRAP_CLAMP_TO_BORDER = 0x812D,
	
	CM_TEXTURE_FILTER_NEAREST = 0x2600,
	CM_TEXTURE_FILTER_LINEAR = 0x2601,
	CM_TEXTURE_FILTER_TRILINEAR = 0x2602,
	
}TextureFlags;

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
typedef struct
{
	vec3 position;    // Translation
	versor rotation;  // Rotation
	vec3 scale;       // Scale
} Transform;

// Camera, defines position/orientation in 3d space
typedef struct
{
	vec3 position;     // Camera position
	vec3 direction;    // Camera target it looks-at
	vec3 up;           // Camera up vector (rotation over its axis)
	float fov;         // Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic
	int projection;    // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
	float nearPlane;
	float farPlane;
} Camera3D;

typedef struct
{
	vec3 direction;
	vec3 color;
	float luminosity;
}GlobalLight;

typedef struct
{
	vec3 normal;    //Normal of the surface
	float distance; //From origin
}Plane;

typedef Plane Frustum[6];

typedef struct
{
	vec3 center;
	vec3 extents;
}BoundingVolume;

typedef struct Ebo
{
	unsigned int id;
	unsigned int dataSize;
	const void* data;
	unsigned int type;
	unsigned int indexCount;
}Ebo;

typedef struct Vbo
{
	unsigned int id;
	unsigned int dataSize;
	unsigned int vertexCount;
	const void* data;
	Ebo ebo;
}Vbo;

typedef struct Ssbo
{
	unsigned int id;
	unsigned int bindingId;
	unsigned int dataSize;
}Ssbo;

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
	Vbo vbo;
	unsigned int stride;
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

typedef enum DrawType
{
	CM_POINTS = 0x0000,
	CM_LINES = 0x0001,
	CM_LINE_LOOP = 0x0002,
	CM_LINE_STRIP = 0x0003,
	CM_TRIANGLES = 0x0004,
	CM_TRIANGLE_STRIPS = 0x0005,
	CM_TRIANGLE_FAN = 0x0006
} DrawType;

//endregion

//region Textures
extern Image cm_load_image(const char* filePath);
extern void cm_unload_image(Image image);
extern void cm_unload_images(Image* image, int size);

extern Texture cm_load_texture(const char* filePath, TextureFlags wrap, TextureFlags filter, bool useMipMaps);
extern void cm_unload_texture(Texture tex);
extern Texture cm_load_texture_from_image(Image image, TextureFlags wrap, TextureFlags filter);
//endregion

//region Shader
extern Shader cm_load_shader(const char *vsPath, const char *fsPath);
extern Shader cm_load_shader_from_memory(const char *vsCode, const char *fsCode);
extern void cm_unload_shader(Shader shader);

extern void cm_begin_shader_mode(Shader shader);
extern void cm_end_shader_mode();

extern int cm_get_uniform_location(Shader shader, const char* name);

extern void cm_set_uniform_m4x4(int id, float* m);

extern void cm_set_uniform_vec4(int id, float* m);
extern void cm_set_uniform_ivec4(int id, int* m);
extern void cm_set_uniform_uvec4(int id, unsigned int* m);

extern void cm_set_uniform_vec3(int id, float* m);
extern void cm_set_uniform_ivec3(int id, int* m);
extern void cm_set_uniform_uvec3(int id, unsigned int* m);

extern void cm_set_uniform_vec2(int id, float* m);
extern void cm_set_uniform_ivec2(int id, int* m);
extern void cm_set_uniform_uvec2(int id, unsigned int* m);

extern void cm_set_uniform_f(int id, float f);
extern void cm_set_uniform_i(int id, int f);
extern void cm_set_uniform_u(int id, unsigned int f);

extern void cm_set_texture(int id, unsigned int texId, unsigned char bindingPoint);
//endregion

//region Camera
extern void cm_begin_mode_3d(Camera3D camera);
extern void cm_end_mode_3d();
extern Frustum* cm_get_frustum();
extern bool cm_is_in_main_frustum(BoundingVolume* volume);
extern bool cm_is_on_or_forward_plane(Plane* plane, BoundingVolume* volume);
extern bool cm_is_on_or_backward_plane(Plane* plane, BoundingVolume* volume);
//endregion

//region Light
extern void cm_set_global_light(GlobalLight light);
//endregion

//region GL Buffers
extern Ssbo cm_load_ssbo(unsigned int bindingId, unsigned int dataSize, const void* data);
extern void cm_upload_ssbo(Ssbo ssbo, unsigned int offset, unsigned int size, const void* data);
extern void cm_unload_ssbo(Ssbo ssbo);
extern bool cm_load_ubo(unsigned int bindingId, unsigned int dataSize, const void* data);
extern void cm_upload_ubos();
extern Vao cm_load_vao(VaoAttribute* attributes, unsigned int attributeCount, Vbo vbo);
extern void cm_unload_vao(Vao vao);

extern Vbo cm_load_vbo(unsigned int dataSize, unsigned int vertexCount, const void* data, Ebo ebo);
extern void cm_unload_vbo(Vbo vbo);
extern void cm_reupload_vbo(Vbo* vbo, unsigned int dataSize, const void* data);
extern void cm_reupload_vbo_partial(Vbo* vbo, unsigned int dataOffset, unsigned int uploadSize);
extern Ebo cm_load_ebo(unsigned int dataSize, const void* data,
					   unsigned int type, unsigned int indexCount);
extern void cm_unload_ebo(Ebo ebo);
extern void cm_reupload_ebo(Ebo* ebo, unsigned int dataSize, const void* data, unsigned int indexCount);

extern void cm_draw_vao(Vao vao, DrawType drawType);
extern void cm_draw_instanced_vao(Vao vao, DrawType drawType, unsigned int instanceCount);
//endregion

//region Drawing
extern Vao cm_get_unit_quad();
//endregion

//region Input Functions

//region Keys
extern bool cm_is_key_pressed(int key);        // Check if a key has been pressed once
extern bool cm_is_key_pressed_repeat(int key); // Check if a key has been pressed again (Only PLATFORM_DESKTOP)
extern bool cm_is_key_down(int key);           // Check if a key is being pressed
extern bool cm_is_key_released(int key);       // Check if a key has been released once
extern bool cm_is_key_up(int key);             // Check if a key is NOT being pressed
extern int cm_get_key_pressed(void);           // Get key pressed (keycode), call it multiple times for keys queued, returns 0 when the queue is empty
extern int cm_get_char_pressed(void);          // Get char pressed (unicode), call it multiple times for chars queued, returns 0 when the queue is empty
//endregion

//region Gamepads
extern bool cm_is_gamepad_available(int gamepad);                    // Check if a gamepad is available
extern const char *cm_get_gamepad_name(int gamepad);                 // Get gamepad internal name id
extern bool cm_is_gamepad_button_pressed(int gamepad, int button);   // Check if a gamepad button has been pressed once
extern bool cm_is_gamepad_button_down(int gamepad, int button);      // Check if a gamepad button is being pressed
extern bool cm_is_gamepad_button_released(int gamepad, int button);  // Check if a gamepad button has been released once
extern bool cm_is_gamepad_button_up(int gamepad, int button);        // Check if a gamepad button is NOT being pressed
extern int cm_get_gamepad_button_pressed(void);                      // Get the last gamepad button pressed
extern int cm_get_gamepad_axis_count(int gamepad);                   // Get gamepad axis count for a gamepad
extern float cm_get_gamepad_axis_movement(int gamepad, int axis);    // Get axis movement value for a gamepad axis
//endregion

//region Mouse
extern bool cm_is_mouse_button_pressed(int button);          // Check if a mouse button has been pressed once
extern bool cm_is_mouse_button_down(int button);             // Check if a mouse button is being pressed
extern bool cm_is_mouse_button_released(int button);         // Check if a mouse button has been released once
extern bool cm_is_mouse_button_up(int button);               // Check if a mouse button is NOT being pressed
extern float cm_get_mouse_X(void);                             // Get mouse position X
extern float cm_get_mouse_Y(void);                             // Get mouse position Y
extern void cm_get_mouse_position(float* position);          // Get mouse position XY
extern void cm_get_mouse_delta(float* delta);                // Get mouse delta between frames
extern void cm_set_mouse_offset(float offsetX, float offsetY);   // Set mouse offset
extern void cm_set_mouse_scale(float scaleX, float scaleY);  // Set mouse scaling
extern float cm_get_mouse_wheel_move(void);                  // Get mouse wheel movement for X or Y, whichever is larger
extern void cm_get_mouse_wheel_move_V(float* v);             // Get mouse wheel movement for both X and Y
//endregion

//region Touch
extern int cm_get_touch_X(void);                                 // Get touch position X for touch point 0 (relative to screen size)
extern int cm_get_touch_Y(void);                                 // Get touch position Y for touch point 0 (relative to screen size)
extern void cm_get_touch_position(int index, float* position);   // Get touch position XY for a touch point index (relative to screen size)
extern int cm_get_touch_point_id(int index);                     // Get touch point identifier for given index
extern int cm_get_touch_point_count(void);                       // Get number of touch points
//endregion

//endregion

//region ThreadPool

extern ThreadPool* cm_create_thread_pool(unsigned int numThreads, uint32_t initialCapacity);
extern void cm_submit_job(ThreadPool* pool, ThreadJob job, bool asLast);
extern void cm_destroy_thread_pool(ThreadPool* pool);

//endregion

//region Time

extern void cm_sleep(double sleepTime);
extern float cm_delta_time_f();
extern double cm_delta_time();
extern double cm_time_since_start();
extern unsigned int cm_get_target_frame_rate();
extern void cm_set_target_frame_rate(unsigned int t);
extern unsigned int cm_frame_rate();
extern double cm_frame_time();

//endregion

//region Helper Functions

extern void cm_print_mat4(vec4* mat);
extern void cm_print_v3(float* v3);
extern void cm_print_quat(float* q);
extern void cm_yp_to_direction(float yaw, float pitch, float* direction);
extern void cm_get_transformation(Transform* trs, vec4* m4);
extern const char *cm_get_file_extension(const char *filePath);
extern char *cm_load_file_text(const char *filePath);
extern void cm_unload_file_text(char *text);
//endregion

#endif //COAL_MINER_H
