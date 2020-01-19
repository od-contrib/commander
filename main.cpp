#ifndef PLATFORM_RETROFW
#include <cstdlib>
#endif
#include <iostream>
#include <unistd.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "def.h"
#include "sdlutils.h"
#include "resourceManager.h"
#include "commander.h"

// Globals
SDL_Surface *ScreenSurface;
SDL_Surface *Globals::g_screen = NULL;
const SDL_Color Globals::g_colorTextNormal = {COLOR_TEXT_NORMAL};
const SDL_Color Globals::g_colorTextTitle = {COLOR_TEXT_TITLE};
const SDL_Color Globals::g_colorTextDir = {COLOR_TEXT_DIR};
const SDL_Color Globals::g_colorTextSelected = {COLOR_TEXT_SELECTED};
std::vector<CWindow *> Globals::g_windows;

int main(int argc, char** argv)
{
    // Avoid crash due to the absence of mouse
    {
        char l_s[]="SDL_NOMOUSE=1";
        putenv(l_s);
    }

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Screen
    ScreenSurface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT * PPU_Y, SCREEN_BPP, SURFACE_FLAGS);
    Globals::g_screen = ScreenSurface;
    if (Globals::g_screen == NULL)
    {
        std::cerr << "SDL_SetVideoMode failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Hide cursor
    SDL_ShowCursor(SDL_DISABLE);

    // Init font
    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create instances
    CResourceManager::instance();
    #ifdef PLATFORM_RETROFW
    CCommander l_commander(PATH_DEFAULT, PATH_DEFAULT_RIGHT);
    #else
    std::string l_path = getenv("HOME");
    static const char kDefaultPath[] = "/Dev/DinguxCommander/test";
    l_path += kDefaultPath;
    if (access(l_path.c_str(), F_OK) != 0)
        l_path.resize(l_path.length() - (sizeof(kDefaultPath) - 1));
    CCommander l_commander(l_path, l_path);
    #endif

    // Main loop
    l_commander.execute();

    //Quit
    SDL_utils::hastalavista();

    return 0;
}
