#include "screen.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "sdlutils.h"

Screen screen;

namespace {

#ifndef USE_SDL2
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
#endif

}

int Screen::init()
{
    const auto &cfg = config();
    screen.w = cfg.disp_width;
    screen.h = cfg.disp_height;
    screen.ppu_x = cfg.disp_ppu_x;
    screen.ppu_y = cfg.disp_ppu_y;
    screen.actual_w = cfg.disp_width * cfg.disp_ppu_x;
    screen.actual_h = cfg.disp_height * cfg.disp_ppu_y;

#ifndef USE_SDL2
    const auto &best = *SDL_GetVideoInfo();
    std::fprintf(stderr,
        "Best video mode reported as: %dx%d bpp=%d hw_available=%u\n",
        best.current_w, best.current_h, best.vfmt->BitsPerPixel,
        best.hw_available);

    // Detect non 320x240/480 screens.
    if (config().disp_autoscale) {
        if (best.current_w >= SCREEN_WIDTH * 2) {
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
        } else if (best.current_w != SCREEN_WIDTH) {
            // E.g. RS07 with 480x272 screen.
            screen.actual_w = screen.w = best.current_w;
            screen.actual_h = screen.h = best.current_h;
            screen.ppu_x = screen.ppu_y = 1;
        }
    }
#endif

#ifdef USE_SDL2
    int window_flags = SDL_WINDOW_RESIZABLE;
    if (config().disp_autoscale) window_flags |= SDL_WINDOW_MAXIMIZED;

    int window_w = screen.actual_w;
    int window_h = screen.actual_h;
    window = SDL_CreateWindow("Commander", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, window_h, window_h, window_flags);
    if (window == nullptr) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return 1;
    }
    if (config().disp_autoscale) {
        const int disp_index = SDL_GetWindowDisplayIndex(window);
        if (disp_index != -1) {
            float hdpi, vdpi;
            if (SDL_GetDisplayDPI(disp_index, /*ddpi=*/nullptr, &hdpi, &vdpi)
                != -1) {
                if (hdpi != 0 && vdpi != 0) {
                    screen.ppu_x = hdpi / 72.0;
                    screen.ppu_y = vdpi / 72.0;
                }
                SDL_Log("Display DPI: %f %f. Scaling factors: %f %f", hdpi,
                    vdpi, screen.ppu_x, screen.ppu_y);
            } else {
                SDL_Log("SDL_GetDisplayDPI failed: %s", SDL_GetError());
            }
        } else {
            SDL_Log("SDL_GetWindowDisplayIndex failed: %s", SDL_GetError());
        }
    }
    SDL_GetWindowSize(window, &window_w, &window_h);
    screen.actual_w = window_w;
    screen.w = screen.actual_w / screen.ppu_x;
    screen.actual_h = window_h;
    screen.h = screen.actual_h / screen.ppu_y;
    screen.surface = SDL_GetWindowSurface(window);
#else
    surface = SetVideoMode(
        screen.actual_w, screen.actual_h, SCREEN_BPP, SDL_SWSURFACE | SDL_RESIZABLE);
    if (surface == nullptr) {
        std::fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return 1;
    }
#endif
    return 0;
}

int Screen::onResize(int w, int h)
{
#ifdef USE_SDL2
    this->surface = SDL_GetWindowSurface(this->window);
    this->actual_w = surface->w;
    this->actual_h = surface->h;
#else
    this->surface = SDL_GetVideoSurface();
    this->actual_w = w;
    this->actual_h = h;
#endif
    this->w = this->actual_w / ppu_x;
    this->h = this->actual_h / ppu_y;
    return 0;
}
