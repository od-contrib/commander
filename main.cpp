#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "error_dialog.h"
#include "commander.h"
#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

// Globals
const SDL_Color Globals::g_colorTextNormal = {COLOR_TEXT_NORMAL};
const SDL_Color Globals::g_colorTextTitle = {COLOR_TEXT_TITLE};
const SDL_Color Globals::g_colorTextDir = {COLOR_TEXT_DIR};
const SDL_Color Globals::g_colorTextSelected = {COLOR_TEXT_SELECTED};
std::vector<CWindow *> Globals::g_windows;

int main(int argc, char** argv)
{
    std::string exec_error;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--show_exec_error") == 0)
        {
            exec_error = argv[++i];
        } else if (std::strcmp(argv[i], "--res-dir") == 0)
        {
            CResourceManager::SetResDir(argv[++i]);
        }
    }

    // Avoid crash due to the absence of mouse
    char l_s[]="SDL_NOMOUSE=1";
    putenv(l_s);

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP) == 0) {
        std::cerr << "IMG_Init failed" << std::endl;
    } else {
        // Clear the errors for image libraries that did not initialize.
        SDL_ClearError();
    }

    // Hide cursor before creating the output surface.
    SDL_ShowCursor(SDL_DISABLE);

    if (screen.init() != 0) return 1;

    // Init font
    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

#ifndef USE_SDL2
    SDL_EnableUNICODE(1);
#endif

    // Create instances
    CResourceManager::instance();

    std::string l_path = PATH_DEFAULT;
    std::string r_path = PATH_DEFAULT_RIGHT;
    if (access(l_path.c_str(), F_OK) != 0) l_path = "/";
#ifdef PATH_DEFAULT_RIGHT_FALLBACK
    if (l_path == r_path || access(r_path.c_str(), F_OK) != 0) r_path = PATH_DEFAULT_RIGHT_FALLBACK;
#endif
    if (access(r_path.c_str(), F_OK) != 0) r_path = "/";

    CCommander l_commander(l_path, r_path);

    if (!exec_error.empty())
        ErrorDialog("Exec error", exec_error);

    // Main loop
    l_commander.execute();

    //Quit
    SDL_utils::hastalavista();

    return 0;
}
