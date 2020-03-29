#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_

#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#define NB_SURFACES 8

class CResourceManager
{
    public:

    typedef enum
    {
        T_SURFACE_FOLDER = 0,
        T_SURFACE_FILE,
        T_SURFACE_FILE_IMAGE,
        T_SURFACE_FILE_INSTALLABLE_PACKAGE,
        T_SURFACE_FILE_PACKAGE,
        T_SURFACE_UP,
        T_SURFACE_CURSOR1,
        T_SURFACE_CURSOR2
    }
    T_SURFACE;

    // Method to get the instance
    static CResourceManager& instance(void);

    // Cleanup all resources
    void sdlCleanup(void);

    // Get a loaded surface
    SDL_Surface *getSurface(const T_SURFACE p_surface) const;

    // Get the loaded fonts
    const std::vector<TTF_Font *> &getFonts(void) const;

    private:

    // Forbidden
    CResourceManager(void);
    CResourceManager(const CResourceManager &p_source);
    const CResourceManager &operator =(const CResourceManager &p_source);

    // Images
    SDL_Surface *m_surfaces[NB_SURFACES];

    // Font
    std::vector<TTF_Font *> m_fonts;
};

#endif
