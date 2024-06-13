#include "input.h"
#include "coal_miner.h"

Input* input_ptr;

void load_input(Input* input)
{
	input_ptr = input;
}

//region Keys
// Check if a key has been pressed once
bool cm_is_key_pressed(int key)
{
	bool pressed = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if ((input_ptr->Keyboard.previousKeyState[key] == 0) && (input_ptr->Keyboard.currentKeyState[key] == 1)) pressed = true;
	}

	return pressed;
}

// Check if a key has been pressed again
bool cm_is_key_pressed_repeat(int key)
{
	bool repeat = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (input_ptr->Keyboard.keyRepeatInFrame[key] == 1) repeat = true;
	}

	return repeat;
}

// Check if a key is being pressed (key held down)
bool cm_is_key_down(int key)
{
	bool down = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (input_ptr->Keyboard.currentKeyState[key] == 1) down = true;
	}

	return down;
}

// Check if a key has been released once
bool cm_is_key_released(int key)
{
	bool released = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if ((input_ptr->Keyboard.previousKeyState[key] == 1) && (input_ptr->Keyboard.currentKeyState[key] == 0)) released = true;
	}

	return released;
}

// Check if a key is NOT being pressed (key not held down)
bool cm_is_key_up(int key)
{
	bool up = false;

	if ((key > 0) && (key < MAX_KEYBOARD_KEYS))
	{
		if (input_ptr->Keyboard.currentKeyState[key] == 0) up = true;
	}

	return up;
}

// Get the last key pressed
int cm_get_key_pressed(void)
{
	int value = 0;

	if (input_ptr->Keyboard.keyPressedQueueCount > 0)
	{
		// Get character from the queue head
		value = input_ptr->Keyboard.keyPressedQueue[0];

		// Shift elements 1 step toward the head
		for (int i = 0; i < (input_ptr->Keyboard.keyPressedQueueCount - 1); i++)
			input_ptr->Keyboard.keyPressedQueue[i] = input_ptr->Keyboard.keyPressedQueue[i + 1];

		// Reset last character in the queue
		input_ptr->Keyboard.keyPressedQueue[input_ptr->Keyboard.keyPressedQueueCount - 1] = 0;
		input_ptr->Keyboard.keyPressedQueueCount--;
	}

	return value;
}

// Get the last char pressed
int cm_get_char_pressed(void)
{
	int value = 0;

	if (input_ptr->Keyboard.charPressedQueueCount > 0)
	{
		// Get character from the queue head
		value = input_ptr->Keyboard.charPressedQueue[0];

		// Shift elements 1 step toward the head
		for (int i = 0; i < (input_ptr->Keyboard.charPressedQueueCount - 1); i++)
			input_ptr->Keyboard.charPressedQueue[i] = input_ptr->Keyboard.charPressedQueue[i + 1];

		// Reset last character in the queue
		input_ptr->Keyboard.charPressedQueue[input_ptr->Keyboard.charPressedQueueCount - 1] = 0;
		input_ptr->Keyboard.charPressedQueueCount--;
	}

	return value;
}
//endregion

//region Gamepads
// Check if a gamepad is available
bool cm_is_gamepad_available(int gamepad)
{
	bool result = false;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad]) result = true;

	return result;
}

// Get gamepad internal name id
const char *cm_get_gamepad_name(int gamepad)
{
	return input_ptr->Gamepad.name[gamepad];
}

// Check if a gamepad button has been pressed once
bool cm_is_gamepad_button_pressed(int gamepad, int button)
{
	bool pressed = false;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad] && (button < MAX_GAMEPAD_BUTTONS) &&
	    (input_ptr->Gamepad.previousButtonState[gamepad][button] == 0) && (input_ptr->Gamepad.currentButtonState[gamepad][button] == 1)) pressed = true;

	return pressed;
}

// Check if a gamepad button is being pressed
bool cm_is_gamepad_button_down(int gamepad, int button)
{
	bool down = false;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad] && (button < MAX_GAMEPAD_BUTTONS) &&
	    (input_ptr->Gamepad.currentButtonState[gamepad][button] == 1)) down = true;

	return down;
}

// Check if a gamepad button has NOT been pressed once
bool cm_is_gamepad_button_released(int gamepad, int button)
{
	bool released = false;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad] && (button < MAX_GAMEPAD_BUTTONS) &&
	    (input_ptr->Gamepad.previousButtonState[gamepad][button] == 1) && (input_ptr->Gamepad.currentButtonState[gamepad][button] == 0)) released = true;

	return released;
}

// Check if a gamepad button is NOT being pressed
bool cm_is_gamepad_button_up(int gamepad, int button)
{
	bool up = false;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad] && (button < MAX_GAMEPAD_BUTTONS) &&
	    (input_ptr->Gamepad.currentButtonState[gamepad][button] == 0)) up = true;

	return up;
}

// Get the last gamepad button pressed
int cm_get_gamepad_button_pressed(void)
{
	return input_ptr->Gamepad.lastButtonPressed;
}

// Get gamepad axis count
int cm_get_gamepad_axis_count(int gamepad)
{
	return input_ptr->Gamepad.axisCount[gamepad];
}

// Get axis movement vector for a gamepad
float cm_get_gamepad_axis_movement(int gamepad, int axis)
{
	float value = 0;

	if ((gamepad < MAX_GAMEPADS) && input_ptr->Gamepad.ready[gamepad] && (axis < MAX_GAMEPAD_AXIS) &&
	    (fabsf(input_ptr->Gamepad.axisState[gamepad][axis]) > 0.1f)) value = input_ptr->Gamepad.axisState[gamepad][axis];      // 0.1f = GAMEPAD_AXIS_MINIMUM_DRIFT/DELTA

	return value;
}
//endregion

//region Mouse
// Check if a mouse button has been pressed once
bool cm_is_mouse_button_pressed(int button)
{
	bool pressed = false;

	if ((input_ptr->Mouse.currentButtonState[button] == 1) && (input_ptr->Mouse.previousButtonState[button] == 0)) pressed = true;

	// Map touches to mouse buttons checking
	if ((input_ptr->Touch.currentTouchState[button] == 1) && (input_ptr->Touch.previousTouchState[button] == 0)) pressed = true;

	return pressed;
}

// Check if a mouse button is being pressed
bool cm_is_mouse_button_down(int button)
{
	bool down = false;

	if (input_ptr->Mouse.currentButtonState[button] == 1) down = true;

	// NOTE: Touches are considered like mouse buttons
	if (input_ptr->Touch.currentTouchState[button] == 1) down = true;

	return down;
}

// Check if a mouse button has been released once
bool cm_is_mouse_button_released(int button)
{
	bool released = false;

	if ((input_ptr->Mouse.currentButtonState[button] == 0) && (input_ptr->Mouse.previousButtonState[button] == 1)) released = true;

	// Map touches to mouse buttons checking
	if ((input_ptr->Touch.currentTouchState[button] == 0) && (input_ptr->Touch.previousTouchState[button] == 1)) released = true;

	return released;
}

// Check if a mouse button is NOT being pressed
bool cm_is_mouse_button_up(int button)
{
	bool up = false;

	if (input_ptr->Mouse.currentButtonState[button] == 0) up = true;

	// NOTE: Touches are considered like mouse buttons
	if (input_ptr->Touch.currentTouchState[button] == 0) up = true;

	return up;
}

// Get mouse position X
float cm_get_mouse_X(void)
{
	return ((input_ptr->Mouse.currentPosition[0] + input_ptr->Mouse.offset[0])*input_ptr->Mouse.scale[0]);
}

// Get mouse position Y
float cm_get_mouse_Y(void)
{
	return ((input_ptr->Mouse.currentPosition[1] + input_ptr->Mouse.offset[1])*input_ptr->Mouse.scale[1]);
}

// Get mouse position XY
void cm_get_mouse_position(float* position)
{
	position[0] = (input_ptr->Mouse.currentPosition[0] + input_ptr->Mouse.offset[0])*input_ptr->Mouse.scale[0];
	position[1] = (input_ptr->Mouse.currentPosition[1] + input_ptr->Mouse.offset[1])*input_ptr->Mouse.scale[1];
}

// Get mouse delta between frames
void cm_get_mouse_delta(float* delta)
{
	delta[0] = input_ptr->Mouse.currentPosition[0] - input_ptr->Mouse.previousPosition[0];
	delta[1] = input_ptr->Mouse.currentPosition[1] - input_ptr->Mouse.previousPosition[1];
}

// Set mouse offset
// NOTE: Useful when rendering to different size targets
void cm_set_mouse_offset(float offsetX, float offsetY)
{
	glm_vec2((vec2) { offsetX, offsetY }, input_ptr->Mouse.offset);
}

// Set mouse scaling
// NOTE: Useful when rendering to different size targets
void cm_set_mouse_scale(float scaleX, float scaleY)
{
	glm_vec2((vec2) { scaleX, scaleY }, input_ptr->Mouse.scale);
}

// Get mouse wheel movement Y
float cm_get_mouse_wheel_move(void)
{
	float result;

	if (fabsf(input_ptr->Mouse.currentWheelMove[0]) > fabsf(input_ptr->Mouse.currentWheelMove[1])) result = (float)input_ptr->Mouse.currentWheelMove[0];
	else result = (float)input_ptr->Mouse.currentWheelMove[1];

	return result;
}

// Get mouse wheel movement X/Y as a vector
void cm_get_mouse_wheel_move_V(float* v)
{
	glm_vec2(input_ptr->Mouse.currentWheelMove, v);
}

// Get touch position X for touch point 0 (relative to screen size)
int cm_get_touch_X(void)
{
	return (int)input_ptr->Touch.position[0][0];
}

// Get touch position Y for touch point 0 (relative to screen size)
int cm_get_touch_Y(void)
{
	return (int)input_ptr->Touch.position[0][1];
}

// Get touch position XY for a touch point index (relative to screen size)
void cm_get_touch_position(int index, float* position)
{

	if (index < MAX_TOUCH_POINTS) glm_vec2(input_ptr->Touch.position[index], position);
	else
	{
		glm_vec2(GLM_VEC2_ONE, position);
		printf("INPUT: Required touch point out of range (Max touch points: %i)", MAX_TOUCH_POINTS);
	}
}

// Get touch point identifier for given index
int cm_get_touch_point_id(int index)
{
	int id = -1;

	if (index < MAX_TOUCH_POINTS) id = input_ptr->Touch.pointId[index];

	return id;
}

// Get number of touch points
int cm_get_touch_point_count(void)
{
	return input_ptr->Touch.pointCount;
}