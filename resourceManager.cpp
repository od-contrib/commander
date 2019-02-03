#include <iostream>
#include "resourceManager.h"
#include "def.h"
#include "sdlutils.h"
#include "def.h"

CResourceManager& CResourceManager::instance(void)
{
    static CResourceManager l_singleton;
    return l_singleton;
}

CResourceManager::CResourceManager(void) :
    m_font(NULL)
{
    // Load images
    m_surfaces[T_SURFACE_BG] = SDL_utils::loadImage("res/background.png");
    m_surfaces[T_SURFACE_FILE] = SDL_utils::loadImage("res/file.png");
    m_surfaces[T_SURFACE_FOLDER] = SDL_utils::loadImage("res/folder.png");
    m_surfaces[T_SURFACE_UP] = SDL_utils::loadImage("res/up.png");
    m_surfaces[T_SURFACE_CURSOR1] = SDL_utils::createImage(159, 15, SDL_MapRGB(Globals::g_screen->format, COLOR_CURSOR_1));
    m_surfaces[T_SURFACE_CURSOR2] = SDL_utils::createImage(159, 15, SDL_MapRGB(Globals::g_screen->format, COLOR_CURSOR_2));
    // Load font
    m_font = SDL_utils::loadFont("res/Fiery_Turk.ttf", 8);
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
