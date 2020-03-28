#include <iostream>

#include <SDL_image.h>
#include <SDL_rotozoom.h>
#include "resourceManager.h"
#include "def.h"
#include "screen.h"
#include "sdlutils.h"

#ifndef FONT_PATH
#define FONT_PATH RES_DIR "wy_scorpio.ttf"
#endif

namespace {

SDL_Surface *LoadIcon(const char *path) {
    SDL_Surface *img = IMG_Load(path);
    if(img == nullptr)
    {
        std::cerr << "LoadIcon(\"" << path << "\"): " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Surface *scaled = nullptr;
    if ((screen.ppu_x == 1 || screen.ppu_x == 2) && (screen.ppu_y == 1 || screen.ppu_y == 2)) {
        scaled = shrinkSurface(img, 2 / screen.ppu_x, 2 / screen.ppu_y);
    } else {
        scaled = zoomSurface(img, screen.ppu_x / 2, screen.ppu_y / 2, SMOOTHING_ON);
    }
    SDL_FreeSurface(img);
    SDL_Surface *display = SDL_DisplayFormatAlpha(scaled);
    SDL_FreeSurface(scaled);
    return display;
}

} // namespace

CResourceManager& CResourceManager::instance(void)
{
    static CResourceManager l_singleton;
    return l_singleton;
}

CResourceManager::CResourceManager(void) :
    m_font(NULL)
{
    // Load images
    m_surfaces[T_SURFACE_FOLDER] = LoadIcon(RES_DIR "folder.png");
    m_surfaces[T_SURFACE_FILE] = LoadIcon(RES_DIR "file-text.png");
    m_surfaces[T_SURFACE_FILE_IMAGE] = LoadIcon(RES_DIR "file-image.png");
    m_surfaces[T_SURFACE_FILE_INSTALLABLE_PACKAGE] = LoadIcon(RES_DIR "file-ipk.png");
    m_surfaces[T_SURFACE_FILE_PACKAGE] = LoadIcon(RES_DIR "file-opk.png");
    m_surfaces[T_SURFACE_UP] = LoadIcon(RES_DIR "up.png");
    m_surfaces[T_SURFACE_CURSOR1] = SDL_utils::createImage(screen.w / 2 * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(Globals::g_screen->format, COLOR_CURSOR_1));
    m_surfaces[T_SURFACE_CURSOR2] = SDL_utils::createImage(screen.w / 2 * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(Globals::g_screen->format, COLOR_CURSOR_2));
    // Load font
    m_font = SDL_utils::loadFont(FONT_PATH, 8);
}

void CResourceManager::sdlCleanup(void)
{
    INHIBIT(std::cout << "CResourceManager::sdlCleanup" << std::endl;)
    int l_i(0);
    // Free surfaces
    for (l_i = 0; l_i < NB_SURFACES; ++l_i)
    {
        if (m_surfaces[l_i] != NULL)
        {
            SDL_FreeSurface(m_surfaces[l_i]);
            m_surfaces[l_i] = NULL;
        }
    }
    // Free font
    if (m_font != NULL)
    {
        TTF_CloseFont(m_font);
        m_font = NULL;
    }
}

SDL_Surface *CResourceManager::getSurface(const T_SURFACE p_surface) const
{
    return m_surfaces[p_surface];
}

TTF_Font *CResourceManager::getFont(void) const
{
    return m_font;
}
