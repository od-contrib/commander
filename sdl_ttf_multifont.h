#ifndef _SDL_TTF_MULTIFONT_H_
#define _SDL_TTF_MULTIFONT_H_

#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

/* Rendering with font fallback for SDL_ttf. */

/* Like TTF_RenderUTF8_Shaded but supports multiple fonts. */
SDL_Surface *TTFMultiFont_RenderUTF8_Shaded(
    const std::vector<TTF_Font *> &fonts, const std::string &text, SDL_Color fg,
    SDL_Color bg);

#endif  // _SDL_TTF_MULTIFONT_H_