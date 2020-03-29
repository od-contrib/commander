#ifndef _SDLUTILS_H_
#define _SDLUTILS_H_

#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>
#include "window.h"

namespace SDL_utils
{
    // Text alignment
    typedef enum
    {
        T_TEXT_ALIGN_LEFT = 0,
        T_TEXT_ALIGN_RIGHT,
        T_TEXT_ALIGN_CENTER
    }
    T_TEXT_ALIGN;

    inline SDL_Rect Rect(Sint16 x, Sint16 y, Uint16 w, Uint16 h)
    {
        return SDL_Rect{x, y, w, h};
    }

    // Load an image to fit the given viewport size.
    SDL_Surface *loadImageToFit(const std::string &p_filename, int fit_w, int fit_h);

    bool isSupportedImageExt(const std::string &filename);

    // Load a TTF font
    TTF_Font *loadFont(const std::string &p_font, const int p_size);

    // Apply a surface on another surface
    void applySurface(const Sint16 p_x, const Sint16 p_y, SDL_Surface* p_source, SDL_Surface* p_destination, SDL_Rect *p_clip = NULL);

    // Render a text
    SDL_Surface *renderText(const std::vector<TTF_Font *> &p_fonts, const std::string &p_text, const SDL_Color &p_fg, const SDL_Color &p_bg);

    // Render a text and apply on a given surface
    void applyText(Sint16 p_x, Sint16 p_y, SDL_Surface* p_destination, const std::vector<TTF_Font *> &p_fonts, const std::string &p_text, const SDL_Color &p_fg, const SDL_Color &p_bg, const T_TEXT_ALIGN p_align = T_TEXT_ALIGN_LEFT);

    // Create a surface in the same format as the screen
    SDL_Surface *createSurface(int width, int height);

    // Create an image filled with the given color
    SDL_Surface *createImage(const int p_width, const int p_height, const Uint32 p_color);

    // Render all opened windows
    void renderAll(void);

    // Cleanup and quit
    void hastalavista(void);

    // Display a waiting window
    void pleaseWait(void);
}

// Globals
namespace Globals
{
    // Screen
    extern SDL_Surface *g_screen;
    // Colors
    extern const SDL_Color g_colorTextNormal;
    extern const SDL_Color g_colorTextTitle;
    extern const SDL_Color g_colorTextDir;
    extern const SDL_Color g_colorTextSelected;
    // The list of opened windows
    extern std::vector<CWindow *> g_windows;
}

#endif
