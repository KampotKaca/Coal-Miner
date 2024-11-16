#ifndef COAL_CONFIG_H
#define COAL_CONFIG_H

#define COAL_PROFILE_MAJOR 4
#define COAL_PROFILE_MINOR 3
#define COAL_PROFILE 0x00032001

#define MAX_KEYBOARD_KEYS                 512       // Maximum number of keyboard keys supported
#define MAX_MOUSE_BUTTONS                   8       // Maximum number of mouse buttons supported
#define MAX_GAMEPADS                        4       // Maximum number of gamepads supported
#define MAX_GAMEPAD_AXIS                    8       // Maximum number of axis supported (per gamepad)
#define MAX_GAMEPAD_BUTTONS                32       // Maximum number of buttons supported (per gamepad)
#define MAX_TOUCH_POINTS                    8       // Maximum number of touch points supported
#define MAX_KEY_PRESSED_QUEUE              16       // Maximum number of keys in the key input queue
#define MAX_CHAR_PRESSED_QUEUE             16       // Maximum number of characters in the char input queue

#define APPLICATION_EXIT_KEY              256
#define MAX_PATH_SIZE                     256
#define MAX_NUM_APPLICATION_ICONS           8
#define MAX_NUM_UBOS                        8
#define FRAME_RATE_RECORD_RATE             60

#define MAX_THREADS_IN_THREAD_POOL         32
#define MAX_SHADER_UNIFORM_NAME_LENGTH     64

#endif //COAL_CONFIG_H
