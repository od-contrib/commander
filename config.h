#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstdlib>
#include <string>

#include "config_def.h"
#include "sdl_backports.h"

struct Config {
    // Display settings
    int disp_width = SCREEN_WIDTH;
    int disp_height = SCREEN_HEIGHT;
    int disp_bpp = SCREEN_BPP;
    float disp_ppu_x = PPU_X;
    float disp_ppu_y = PPU_Y;
    bool disp_autoscale = static_cast<bool>(AUTOSCALE);

    // Default paths to the left and right panels
    std::string path_default = std::getenv("PWD");        // $PWD in the config
    std::string path_default_right = std::getenv("HOME"); // $HOME in the config

    // Used if `path_default_right` does not exist or left path == right path.
    std::string path_default_right_fallback;

    // Default filesystem for Disk info
    std::string file_system = FILE_SYSTEM;

    // Resources directory (e.g. icons).
    std::string res_dir { RES_DIR };

    // Keyboard key code mappings
    SDLC_Keycode key_down = MYKEY_DOWN;
    SDLC_Keycode key_left = MYKEY_LEFT;
    SDLC_Keycode key_open = MYKEY_OPEN;
    SDLC_Keycode key_operation = MYKEY_OPERATION;
    SDLC_Keycode key_pagedown = MYKEY_PAGEDOWN;
    SDLC_Keycode key_pageup = MYKEY_PAGEUP;
    SDLC_Keycode key_parent = MYKEY_PARENT;
    SDLC_Keycode key_right = MYKEY_RIGHT;
    SDLC_Keycode key_select = MYKEY_SELECT;
    SDLC_Keycode key_system = MYKEY_SYSTEM;
    SDLC_Keycode key_transfer = MYKEY_TRANSFER;
    SDLC_Keycode key_up = MYKEY_UP;

    // On-screen keyboard settings.
#ifdef OSK_KEY_SYSTEM_IS_BACKSPACE
    // `key_system` is backspace, `key_parent` is cancel
    bool osk_key_system_is_backspace = true;
#else
    // `key_system` is cancel, `key_parent` is backspace
    bool osk_key_system_is_backspace = false;
#endif

    void Load(const std::string &path);
};

Config &config();

#endif // CONFIG_H_
