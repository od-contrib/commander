#ifndef _DEF_H_
#define _DEF_H_

//~ #define INHIBIT(X) X
#define INHIBIT(X) /* X */

#ifndef PPU_Y
#define PPU_Y 1
#endif
#ifndef PPU_X
#define PPU_X 1
#endif

#ifndef RES_DIR
#define RES_DIR "res/"
#endif

#ifndef FONTS
#define FONTS {RES_DIR"Fiery_Turk.ttf",8},{RES_DIR"FreeSans.ttf",10},{RES_DIR"DroidSansFallbackFull.ttf",9}
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

#define SURFACE_FLAGS SDL_SWSURFACE
#define MS_PER_FRAME 33

#ifndef PATH_DEFAULT
#define PATH_DEFAULT getenv("PWD")
#endif

#ifndef PATH_DEFAULT_RIGHT
#define PATH_DEFAULT_RIGHT getenv("HOME")
#endif

#ifndef FILE_SYSTEM
#define FILE_SYSTEM "/dev/sda4"
#endif

// Panel
#define HEADER_H 17
#define HEADER_PADDING_TOP 3

#define FOOTER_H 13
#define FOOTER_PADDING_TOP 1
#define FOOTER_Y (screen.h - FOOTER_H)

#define Y_LIST HEADER_H

#define LINE_HEIGHT 15
#define NB_VISIBLE_LINES ((screen.h - FOOTER_H - HEADER_H - 1) / LINE_HEIGHT + 1)
#define NB_FULLY_VISIBLE_LINES ((screen.h - FOOTER_H - HEADER_H) / LINE_HEIGHT)

// Dialogs
#define DIALOG_BORDER 2
#define DIALOG_MARGIN 8
// Colors
#define COLOR_TITLE_BG 102, 85, 74
#define COLOR_KEY 255, 0, 255
#define COLOR_TEXT_NORMAL 70, 27, 10
#define COLOR_TEXT_TITLE 233, 229, 227
#define COLOR_TEXT_DIR 75, 70, 164
#define COLOR_TEXT_SELECTED 255, 0, 0
#define COLOR_CURSOR_1 232, 152, 80
#define COLOR_CURSOR_2 232, 201, 173
#define COLOR_BG_1 255, 255, 255
#define COLOR_BG_2 232, 228, 224
#define COLOR_BORDER 102, 85, 74

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
#define MYKEY_SYSTEM SDLK_q
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
#define MYKEY_OPERATION SDLK_a
#endif
#ifndef MYKEY_SELECT
#define MYKEY_SELECT SDLK_INSERT
#endif
#ifndef MYKEY_TRANSFER
#define MYKEY_TRANSFER SDLK_w
#endif

#endif // _DEF_H_
