#include <iostream>

#include <SDL_image.h>
#include <SDL_rotozoom.h>
#include "resourceManager.h"
#include "def.h"
#include "screen.h"
#include "sdlutils.h"

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

CResourceManager::CResourceManager(void)
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
    
    // Load fonts
    struct FontSpec {
        const char *const path;
        int size;
    };
    constexpr FontSpec kFonts[] = {FONTS};
    constexpr std::size_t kFontsLen = sizeof(kFonts) / sizeof(kFonts[0]);
    m_fonts.reserve(kFontsLen);
    for (std::size_t i = 0; i < kFontsLen; ++i) {
        auto *font = SDL_utils::loadFont(kFonts[i].path, kFonts[i].size);
        if (font != nullptr) m_fonts.push_back(font);
    }
    if (m_fonts.empty()) {
        std::cerr << "No fonts found!" << std::endl;
        exit(1);
    }
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
    for (auto *font : m_fonts)
        TTF_CloseFont(font);
    m_fonts.clear();
}

SDL_Surface *CResourceManager::getSurface(const T_SURFACE p_surface) const
{
    return m_surfaces[p_surface];
}

const std::vector<TTF_Font *> &CResourceManager::getFonts(void) const
{
    return m_fonts;
}
