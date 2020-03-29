#include "sdlutils.h"

#include <algorithm>
#include <iostream>

#include <SDL_image.h>
#include <SDL_rotozoom.h>
#include "def.h"
#include "fileutils.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdl_ttf_multifont.h"

extern SDL_Surface *ScreenSurface;

bool SDL_utils::isSupportedImageExt(const std::string &ext) {
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "ico" || ext == "bmp" || ext == "xcf";
}

SDL_Surface *SDL_utils::loadImageToFit(const std::string &p_filename, int fit_w, int fit_h)
{
    // Load image
    SDL_Surface *l_img = IMG_Load(p_filename.c_str());
    if (IMG_GetError() != nullptr && *IMG_GetError() != '\0') {
        if (!strcmp(IMG_GetError(), "Unsupported image format") == 0)
            std::cerr << "SDL_utils::loadImageToFit: " << IMG_GetError() << std::endl;
        SDL_ClearError();
        return nullptr;
    }
    const double aspect_ratio = static_cast<double>(l_img->w) / l_img->h;
    int target_w, target_h;
    if (fit_w * l_img->h <= fit_h * l_img->w) {
        target_w = std::min(l_img->w, fit_w);
        target_h = target_w / aspect_ratio;
    } else {
        target_h = std::min(l_img->h, fit_h);
        target_w = target_h * aspect_ratio;
    }
    target_w *= screen.ppu_x;
    target_h *= screen.ppu_y;
    SDL_Surface *l_img2 = zoomSurface(l_img, static_cast<double>(target_w) / l_img->w, static_cast<double>(target_h) / l_img->h, SMOOTHING_ON);
    SDL_FreeSurface(l_img);

    const std::string ext = File_utils::getLowercaseFileExtension(p_filename);
    const bool supports_alpha = ext != "xcf" && ext != "jpg" && ext != "jpeg";
    SDL_Surface *l_img3 = supports_alpha ? SDL_DisplayFormatAlpha(l_img2) : SDL_DisplayFormat(l_img2);
    SDL_FreeSurface(l_img2);
    return l_img3;
}

void SDL_utils::applySurface(const Sint16 p_x, const Sint16 p_y, SDL_Surface* p_source, SDL_Surface* p_destination, SDL_Rect *p_clip)
{
    // Rectangle to hold the offsets
    SDL_Rect l_offset;
    // Set offsets
    l_offset.x = p_x * screen.ppu_x;
    l_offset.y = p_y * screen.ppu_y;
    //Blit the surface
    SDL_BlitSurface(p_source, p_clip, p_destination, &l_offset);
}

TTF_Font *SDL_utils::loadFont(const std::string &p_font, const int p_size)
{
    INHIBIT(std::cout << "SDL_utils::loadFont(" << p_font << ", " << p_size << ")" << std::endl;)
#ifdef USE_TTF_OPENFONT_DPI
    TTF_Font *l_font = TTF_OpenFontDPI(p_font.c_str(), p_size, 72 * screen.ppu_x, 72 * screen.ppu_y);
#else
    TTF_Font *l_font = TTF_OpenFont(p_font.c_str(), p_size);
#endif
    if (l_font == NULL) {
        std::cerr << "SDL_utils::loadFont: " << SDL_GetError() << std::endl;
        SDL_ClearError();
    }
    return l_font;
}

SDL_Surface *SDL_utils::renderText(const std::vector<TTF_Font *> &p_fonts, const std::string &p_text, const SDL_Color &p_fg, const SDL_Color &p_bg)
{
    SDL_Surface *result = TTFMultiFont_RenderUTF8_Shaded(p_fonts, p_text, p_fg, p_bg);
    if (result == nullptr) {
        std::cerr << "TTFMultiFont_RenderUTF8_Shaded: " << SDL_GetError() << std::endl;
        SDL_ClearError();
    }
    return result;
}

void SDL_utils::applyText(Sint16 p_x, Sint16 p_y, SDL_Surface* p_destination, const std::vector<TTF_Font *> &p_fonts, const std::string &p_text, const SDL_Color &p_fg, const SDL_Color &p_bg, const T_TEXT_ALIGN p_align)
{
    SDL_Surface *l_text = renderText(p_fonts, p_text, p_fg, p_bg);
    switch (p_align)
    {
        case T_TEXT_ALIGN_LEFT:
            applySurface(p_x, p_y, l_text, p_destination);
            break;
        case T_TEXT_ALIGN_RIGHT:
            applySurface(p_x - l_text->w / screen.ppu_x, p_y, l_text, p_destination);
            break;
        case T_TEXT_ALIGN_CENTER:
            applySurface(p_x - l_text->w / 2 / screen.ppu_x, p_y, l_text, p_destination);
            break;
        default:
            break;
    }
    SDL_FreeSurface(l_text);
}

SDL_Surface *SDL_utils::createSurface(int width, int height)
{
    return SDL_CreateRGBSurface(SURFACE_FLAGS, width, height, Globals::g_screen->format->BitsPerPixel, Globals::g_screen->format->Rmask, Globals::g_screen->format->Gmask, Globals::g_screen->format->Bmask, Globals::g_screen->format->Amask);
}

SDL_Surface *SDL_utils::createImage(const int p_width, const int p_height, const Uint32 p_color)
{
    SDL_Surface *l_ret = createSurface(p_width, p_height);
    if (l_ret == NULL)
        std::cerr << "SDL_utils::createImage: " << SDL_GetError() << std::endl;
    // Fill image with the given color
    SDL_FillRect(l_ret, NULL, p_color);
    return l_ret;
}

void SDL_utils::renderAll(void)
{
    if (Globals::g_windows.empty())
        return;
    // First window to draw is the last fullscreen
    unsigned int l_i = Globals::g_windows.size() - 1;
    while (l_i && !Globals::g_windows[l_i]->isFullScreen())
        --l_i;
    // Draw windows
    for (std::vector<CWindow *>::iterator l_it = Globals::g_windows.begin() + l_i; l_it != Globals::g_windows.end(); ++l_it)
        (*l_it)->render(l_it + 1 == Globals::g_windows.end());
}

void SDL_utils::hastalavista(void)
{
    // Destroy all dialogs except the first one (the commander)
    while (Globals::g_windows.size() > 1)
        delete Globals::g_windows.back();
    // Free resources
    CResourceManager::instance().sdlCleanup();
    // Quit SDL
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void SDL_utils::pleaseWait(void)
{
    SDL_Surface *l_surfaceTmp = renderText(CResourceManager::instance().getFonts(), "Please wait...", Globals::g_colorTextNormal, {COLOR_BG_1});
    SDL_Rect l_rect = Rect(
        (screen.w * screen.ppu_x - (l_surfaceTmp->w + (2 * DIALOG_MARGIN + 2 * DIALOG_BORDER) * screen.ppu_x)) / 2,
        (screen.h * screen.ppu_y - (l_surfaceTmp->h + 9)) / 2,
        l_surfaceTmp->w + (2 * DIALOG_MARGIN + 2 * DIALOG_BORDER) * screen.ppu_x,
        l_surfaceTmp->h + 4 * screen.ppu_y);
    SDL_FillRect(Globals::g_screen, &l_rect, SDL_MapRGB(Globals::g_screen->format, COLOR_BORDER));
    l_rect.x += DIALOG_BORDER * screen.ppu_x;
    l_rect.y += DIALOG_BORDER * screen.ppu_y;
    l_rect.w -= 2 * DIALOG_BORDER * screen.ppu_x;
    l_rect.h -= 2 * DIALOG_BORDER * screen.ppu_y;
    SDL_FillRect(Globals::g_screen, &l_rect, SDL_MapRGB(Globals::g_screen->format, COLOR_BG_1));
    applySurface((screen.w - l_surfaceTmp->w / screen.ppu_x) / 2, (screen.h - l_surfaceTmp->h / screen.ppu_y) / 2, l_surfaceTmp, Globals::g_screen);
    SDL_FreeSurface(l_surfaceTmp);
    //SDL_Flip(Globals::g_screen);
    //SDL_Flip(Globals::g_screen);
    SDL_Flip(ScreenSurface);
}
