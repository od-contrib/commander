#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <SDL.h>
#include "def.h"

struct Screen {
  // Logical width and height.
  decltype(SDL_Rect().w) w = SCREEN_WIDTH;
  decltype(SDL_Rect().h) h = SCREEN_HEIGHT;

  // Scaling factors.
  float ppu_x = PPU_X;
  float ppu_y = PPU_Y;

  // Actual width and height.
  decltype(SDL_Rect().w) actual_w = SCREEN_WIDTH * PPU_X;
  decltype(SDL_Rect().h) actual_h = SCREEN_HEIGHT * PPU_Y;
};

extern Screen screen;

#endif // _SCREEN_H_
