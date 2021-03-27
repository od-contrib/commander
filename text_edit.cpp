#include "text_edit.h"

#include <cmath>
#include <cstdio>

#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

namespace {
using SDL_utils::mapRGB;
using SDL_utils::renderRectWithBorder;

bool is_ascii(unsigned char c) { return c <= 0x7f; }
} // namespace

void TextEdit::setDimensions(int width, int height)
{
    width_ = width;
    height_ = height;

    border_width_x_ = 2;
    border_width_y_ = border_width_x_ * std::round(screen.ppu_y / screen.ppu_x);

    padding_x_ = 5 * screen.ppu_x;
    padding_y_ = 3 * screen.ppu_y;

    foreground_rect_.x = border_width_x_;
    foreground_rect_.y = border_width_y_;
    foreground_rect_.w = width_ - 2 * border_width_x_;
    foreground_rect_.h = height_ - 2 * border_width_y_;

    prepareSurfaces();
    update_foreground_ = true;
}

void TextEdit::prepareSurfaces()
{
    background_.reset(SDL_utils::createSurface(width_, height_));
    const auto *pixel_format = background_->format;
    border_color_ = mapRGB(pixel_format, sdl_border_color_);
    bg_color_ = mapRGB(pixel_format, sdl_bg_color_);
    updateBackground();

    foreground_.reset(
        SDL_utils::createSurface(foreground_rect_.w, foreground_rect_.h));
}

void TextEdit::updateBackground()
{
    SDL_Rect bg_rect;
    bg_rect.x = 0;
    bg_rect.y = 0;
    bg_rect.w = width_;
    bg_rect.h = height_;
    renderRectWithBorder(background_.get(), bg_rect, border_width_x_,
        border_width_y_, border_color_, bg_color_);
}

void TextEdit::updateForeground() const
{
    update_foreground_ = false;
    SDL_FillRect(foreground_.get(), nullptr, bg_color_);
    if (text_.empty()) return;
    SDLSurfaceUniquePtr tmp_surface { SDL_utils::renderText(
        CResourceManager::instance().getFonts(), text_,
        Globals::g_colorTextNormal, { COLOR_BG_1 }) };
    const int max_w = foreground_rect_.w - 2 * padding_x_;
    if (tmp_surface->w > foreground_rect_.w) {
        // Text is too big => clip it
        SDL_Rect rect;
        rect.x = tmp_surface->w - max_w;
        rect.y = 0;
        rect.w = max_w;
        rect.h = tmp_surface->h;
        SDL_utils::applyPpuScaledSurface(padding_x_, padding_y_,
            tmp_surface.get(), foreground_.get(), &rect);
        cursor_x_ = rect.w + 1;
    } else {
        SDL_utils::applyPpuScaledSurface(
            padding_x_, padding_y_, tmp_surface.get(), foreground_.get());
        cursor_x_ = tmp_surface->w;
    }
}

void TextEdit::blitBackground(SDL_Surface &out, int x, int y) const
{
    SDL_utils::applyPpuScaledSurface(x, y, background_.get(), &out);
}

void TextEdit::blitForeground(SDL_Surface &out, int x, int y) const
{
    if (update_foreground_) updateForeground();
    SDL_utils::applyPpuScaledSurface(
        x + border_width_x_, y + border_width_y_, foreground_.get(), &out);

    SDL_Rect cursor_rect;
    cursor_rect.x = x + padding_x_ + border_width_x_ + cursor_x_;
    cursor_rect.y = y + padding_y_ + border_width_y_;
    cursor_rect.w = 1;
    cursor_rect.h = foreground_rect_.h - 2 * padding_y_;
    SDL_FillRect(&out, &cursor_rect, border_color_);
}

void TextEdit::appendText(const std::string &text)
{
    if (!text.empty()) update_foreground_ = true;
    text_.append(text);
}

void TextEdit::appendText(char c)
{
    update_foreground_ = true;
    text_ += c;
}

bool TextEdit::backspace()
{
    if (text_.empty()) return false;
    while (!text_.empty() && !is_ascii(text_.back())) text_.pop_back();
    text_.pop_back();
    update_foreground_ = true;
    return true;
}
