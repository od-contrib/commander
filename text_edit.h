#ifndef TEXT_EDIT_H_
#define TEXT_EDIT_H_

#include <string>

#include <SDL.h>

#include "def.h"
#include "sdl_ptrs.h"

class TextEdit {
  public:
    void setDimensions(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }

    void blitBackground(SDL_Surface &out, int x, int y) const;
    void blitForeground(SDL_Surface &out, int x, int y) const;

    const std::string &text() const { return text_; }
    void appendText(const std::string &text);
    void appendText(char c);
    bool backspace();

  private:
    void prepareSurfaces();
    void updateBackground();
    void updateForeground() const;

    std::string text_;

    // Rendering composes background, foreground, and cursor.
    //
    // Cached surfaces:
    mutable SDLSurfaceUniquePtr background_, foreground_;
    mutable int cursor_x_; // int the foreground rect
    mutable bool update_foreground_ = false;

    SDL_Rect foreground_rect_;

    int width_, height_;

    int border_width_x_, border_width_y_, padding_x_, padding_y_;

    SDL_Color sdl_border_color_ = SDL_Color { COLOR_BORDER };
    std::uint32_t border_color_;

    SDL_Color sdl_bg_color_ = SDL_Color { COLOR_BG_1 };
    std::uint32_t bg_color_;
};

#endif // TEXT_EDIT_H_
