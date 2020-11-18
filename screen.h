#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "def.h"
#include <SDL.h>

struct Screen
{
    // Logical width and height.
    decltype(SDL_Rect().w) w = SCREEN_WIDTH;
    decltype(SDL_Rect().h) h = SCREEN_HEIGHT;

    // Scaling factors.
    float ppu_x = PPU_X;
    float ppu_y = PPU_Y;

    // Actual width and height.
    decltype(SDL_Rect().w) actual_w = SCREEN_WIDTH * PPU_X;
    decltype(SDL_Rect().h) actual_h = SCREEN_HEIGHT * PPU_Y;

    // We target 25 FPS because currently the key repeat timer is tied into the
    // frame limit. :(
    int refreshRate = 25;

    SDL_Surface *surface;

    void flip() {
      SDL_Flip(surface);
      surface = SDL_GetVideoSurface();
    }

    // Called once at startup.
    int init();

    // Called on every SDL_RESIZE event.
    int onResize(int w, int h);
};

extern Screen screen;

#endif // _SCREEN_H_
