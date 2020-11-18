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

struct FontSpec {
    const char *const path;
    int size;
};
static constexpr FontSpec kFonts[] = {FONTS};
static constexpr std::size_t kFontsLen = sizeof(kFonts) / sizeof(kFonts[0]);
static constexpr FontSpec kLowDpiFonts[] = {LOW_DPI_FONTS};
static constexpr std::size_t kLowDpiFontsLen = sizeof(kLowDpiFonts) / sizeof(kLowDpiFonts[0]);

std::vector<TTF_Font *> LoadFonts(bool low_dpi) {
    const FontSpec *specs = low_dpi ? kLowDpiFonts : kFonts;
    const std::size_t len = low_dpi ? kLowDpiFontsLen : kFontsLen;

    std::vector<TTF_Font *> fonts;
    fonts.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        auto *font = SDL_utils::loadFont(specs[i].path, specs[i].size);
        if (font != nullptr) fonts.push_back(font);
    }
    if (fonts.empty()) {
        std::cerr << "No fonts found!" << std::endl;
        exit(1);
    }
    return fonts;
}

bool ShouldUseLowDpiFonts() {
    return screen.ppu_x <= 1.0 && kFonts[0].size < 12;
}

} // namespace

CResourceManager& CResourceManager::instance()
{
    static CResourceManager l_singleton;
    return l_singleton;
}

CResourceManager::CResourceManager()
    : m_low_dpi_fonts(ShouldUseLowDpiFonts())
    , m_fonts(LoadFonts(m_low_dpi_fonts))
{
    // Load images
    m_surfaces[T_SURFACE_FOLDER] = LoadIcon(RES_DIR "folder.png");
    m_surfaces[T_SURFACE_FILE] = LoadIcon(RES_DIR "file-text.png");
    m_surfaces[T_SURFACE_FILE_IMAGE] = LoadIcon(RES_DIR "file-image.png");
    m_surfaces[T_SURFACE_FILE_INSTALLABLE_PACKAGE] = LoadIcon(RES_DIR "file-ipk.png");
    m_surfaces[T_SURFACE_FILE_PACKAGE] = LoadIcon(RES_DIR "file-opk.png");
    m_surfaces[T_SURFACE_FILE_IS_SYMLINK] = LoadIcon(RES_DIR "file-is-symlink.png");
    m_surfaces[T_SURFACE_UP] = LoadIcon(RES_DIR "up.png");
    onResize();
}

void CResourceManager::onResize() {
    m_surfaces[T_SURFACE_CURSOR1] = SDL_utils::createImage(screen.w / 2 * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(screen.surface->format, COLOR_CURSOR_1));
    m_surfaces[T_SURFACE_CURSOR2] = SDL_utils::createImage(screen.w / 2 * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(screen.surface->format, COLOR_CURSOR_2));
    if (m_low_dpi_fonts != ShouldUseLowDpiFonts()) {
        m_low_dpi_fonts = !m_low_dpi_fonts;
        closeFonts();
        m_fonts = Fonts{LoadFonts(m_low_dpi_fonts)};
    }
}

void CResourceManager::sdlCleanup() {
    // Free surfaces
    for (std::size_t l_i = 0; l_i < NB_SURFACES; ++l_i)
    {
        if (m_surfaces[l_i] != NULL)
        {
            SDL_FreeSurface(m_surfaces[l_i]);
            m_surfaces[l_i] = NULL;
        }
    }
    closeFonts();
}

void CResourceManager::closeFonts() {
    for (auto &font : m_fonts.fonts()) {
        if (font != nullptr) {
            TTF_CloseFont(font);
            font = nullptr;
        }
    }
}

SDL_Surface *CResourceManager::getSurface(const T_SURFACE p_surface) const
{
    return m_surfaces[p_surface];
}

const Fonts &CResourceManager::getFonts() const
{
    return m_fonts;
}
