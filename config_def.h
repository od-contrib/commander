#ifndef CONFIG_DEF_H_
#define CONFIG_DEF_H_

// Defines for config defaults, so that they can be overriden via the build
// system.

#ifndef RES_DIR
#define RES_DIR "res/"
#endif

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 320
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

#ifndef SCREEN_BPP
#define SCREEN_BPP 16
#endif

#ifndef PPU_Y
#define PPU_Y 1
#endif
#ifndef PPU_X
#define PPU_X 1
#endif

#ifndef PATH_DEFAULT
#define PATH_DEFAULT getenv("PWD")
#endif

#ifndef PATH_DEFAULT_RIGHT
#define PATH_DEFAULT_RIGHT getenv("HOME")
#endif

#ifndef FILE_SYSTEM
#define FILE_SYSTEM "/dev/sda4"
#endif

#ifndef MYKEY_UP
#define MYKEY_UP SDLK_UP
#endif
#ifndef MYKEY_RIGHT
#define MYKEY_RIGHT SDLK_RIGHT
#endif
#ifndef MYKEY_DOWN
#define MYKEY_DOWN SDLK_DOWN
#endif
#ifndef MYKEY_LEFT
#define MYKEY_LEFT SDLK_LEFT
#endif
#ifndef MYKEY_SYSTEM
#define MYKEY_SYSTEM SDLK_ESCAPE
#endif
#ifndef MYKEY_PAGEUP
#define MYKEY_PAGEUP SDLK_PAGEUP
#endif
#ifndef MYKEY_PAGEDOWN
#define MYKEY_PAGEDOWN SDLK_PAGEDOWN
#endif
#ifndef MYKEY_OPEN
#define MYKEY_OPEN SDLK_RETURN
#endif
#ifndef MYKEY_PARENT
#define MYKEY_PARENT SDLK_BACKSPACE
#endif
#ifndef MYKEY_OPERATION
#define MYKEY_OPERATION SDLK_SPACE
#endif
#ifndef MYKEY_SELECT
#define MYKEY_SELECT SDLK_INSERT
#endif
#ifndef MYKEY_TRANSFER
#define MYKEY_TRANSFER SDLK_TAB
#endif

#endif // CONFIG_DEF_H_
