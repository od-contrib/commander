#include "screen.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "sdlutils.h"

Screen screen;

namespace
{

SDL_Surface *SetVideoMode(int width, int height, int bpp, std::uint32_t flags)
{
    std::fprintf(stderr, "Setting video mode %dx%d bpp=%u flags=0x%08X\n",
        width, height, bpp, flags);
    std::fflush(stderr);
    auto *result = SDL_SetVideoMode(width, height, bpp, flags);
    const auto &current = *SDL_GetVideoInfo();
    std::fprintf(stderr, "Video mode is now %dx%d bpp=%u flags=0x%08X\n",
        current.current_w, current.current_h, current.vfmt->BitsPerPixel,
        SDL_GetVideoSurface()->flags);
    std::fflush(stderr);
    return result;
}

}

int Screen::init()
{
    // Screen
    const auto &best = *SDL_GetVideoInfo();
    std::fprintf(stderr,
        "Best video mode reported as: %dx%d bpp=%d hw_available=%u\n",
        best.current_w, best.current_h, best.vfmt->BitsPerPixel,
        best.hw_available);

    // Detect non 320x240/480 screens.
#if AUTOSCALE == 1
    if (best.current_w >= SCREEN_WIDTH * 2)
    {
        // E.g. 640x480. Upscale to the smaller of the two.
        double scale
            = std::min(best.current_w / static_cast<double>(SCREEN_WIDTH),
                best.current_h / static_cast<double>(SCREEN_HEIGHT));
        scale = std::min(scale, 2.0);
        screen.ppu_x = screen.ppu_y = scale;
        screen.w = best.current_w / scale;
        screen.h = best.current_h / scale;
        screen.actual_w = best.current_w;
        screen.actual_h = best.current_h;
    }
    else if (best.current_w != SCREEN_WIDTH)
    {
        // E.g. RS07 with 480x272 screen.
        screen.actual_w = screen.w = best.current_w;
        screen.actual_h = screen.h = best.current_h;
        screen.ppu_x = screen.ppu_y = 1;
    }
#endif
    surface = SetVideoMode(
        screen.actual_w, screen.actual_h, SCREEN_BPP, SURFACE_FLAGS);
    if (surface == nullptr)
    {
        std::fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return 1;
    }
    return 0;
}

int Screen::onResize(int w, int h)
{
    this->surface = SDL_SetVideoMode(w, h, SCREEN_BPP, SURFACE_FLAGS);
    this->actual_w = surface->w;
    this->actual_h = surface->h;
    this->w = this->actual_w / ppu_x;
    this->h = this->actual_h / ppu_y;
    return 0;
}
