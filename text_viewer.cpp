#include "text_viewer.h"

#include <algorithm>
#include <cstring>
#include <fstream>

#include "def.h"
#include "error_dialog.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"
#include "utf8.h"

#define VIEWER_MARGIN 1
#define VIEWER_LINE_HEIGHT 13
#define VIEWER_Y_LIST 18
#define VIEWER_NB_LINES                                                        \
    ((screen.h - VIEWER_Y_LIST - 1) / VIEWER_LINE_HEIGHT + 1)
#define VIEWER_X_STEP 32

TextViewer::TextViewer(std::string filename)
    : fonts_(CResourceManager::instance().getFonts())
    , filename_(std::move(filename))
    , first_line_(0)
{
    std::ifstream input_file(filename_.c_str());
    if (!input_file.is_open()) {
        ErrorDialog(
            "Unable to open file", filename_ + "\n" + std::strerror(errno));
        m_retVal = -1;
        return;
    }
    while (!input_file.eof()) {
        lines_.emplace_back();
        std::getline(input_file, lines_.back());
        utf8::replaceTabsWithSpaces(&lines_.back());
    }
    input_file.close();
    clip_.x = clip_.y = 0;
    init();
}

void TextViewer::init()
{
    const auto &fonts = CResourceManager::instance().getFonts();

    // Create background image
    background_ = SDLSurfaceUniquePtr { SDL_utils::createImage(screen.actual_w,
        screen.actual_h, SDL_MapRGB(screen.surface->format, COLOR_BG_1)) };
    {
        SDL_Rect rect = SDL_utils::Rect(
            0, 0, screen.w * screen.ppu_x, HEADER_H * screen.ppu_y);
        SDL_FillRect(background_.get(), &rect,
            SDL_MapRGB(background_->format, COLOR_BORDER));
    }
    // Print title
    {
        SDLSurfaceUniquePtr tmp { SDL_utils::renderText(
            fonts, filename_, Globals::g_colorTextTitle, { COLOR_TITLE_BG }) };
        if (tmp->w > background_->w - 2 * VIEWER_MARGIN) {
            SDL_Rect rect;
            rect.x = tmp->w - (background_->w - 2 * VIEWER_MARGIN);
            rect.y = 0;
            rect.w = background_->w - 2 * VIEWER_MARGIN;
            rect.h = tmp->h;
            SDL_utils::applyPpuScaledSurface(VIEWER_MARGIN * screen.ppu_x,
                HEADER_PADDING_TOP * screen.ppu_y, tmp.get(), background_.get(),
                &rect);
        } else {
            SDL_utils::applyPpuScaledSurface(VIEWER_MARGIN * screen.ppu_x,
                HEADER_PADDING_TOP * screen.ppu_y, tmp.get(),
                background_.get());
        }
    }
    clip_.w = screen.actual_w - 2 * VIEWER_MARGIN * screen.ppu_x;
}

void TextViewer::onResize() { init(); }

void TextViewer::render(const bool focused) const
{
    SDL_utils::applyPpuScaledSurface(0, 0, background_.get(), screen.surface);

    std::size_t i = std::min(first_line_ + VIEWER_NB_LINES, lines_.size());
    SDL_Rect clip = clip_;
    while (i-- > first_line_) {
        const std::string &line = lines_[i];
        if (line.empty()) continue;
        SDLSurfaceUniquePtr tmp { SDL_utils::renderText(
            fonts_, line, Globals::g_colorTextNormal, { COLOR_BG_1 }) };
        if (tmp == nullptr) continue;
        clip.h = tmp->h;
        SDL_utils::applyPpuScaledSurface(VIEWER_MARGIN * screen.ppu_x,
            (VIEWER_Y_LIST + (i - first_line_) * VIEWER_LINE_HEIGHT)
                * screen.ppu_y,
            tmp.get(), screen.surface, &clip);
    }
}

const bool TextViewer::keyPress(const SDL_Event &event)
{
    CWindow::keyPress(event);
    switch (event.key.keysym.sym) {
        case MYKEY_SYSTEM:
        case MYKEY_PARENT:
            m_retVal = -1;
            return true;
            break;
        case MYKEY_UP: return moveUp(1); break;
        case MYKEY_DOWN: return moveDown(1); break;
        case MYKEY_PAGEUP: return moveUp(VIEWER_NB_LINES - 1); break;
        case MYKEY_PAGEDOWN: return moveDown(VIEWER_NB_LINES - 1); break;
        case MYKEY_LEFT: return moveLeft(); break;
        case MYKEY_RIGHT: return moveRight(); break;
        default: break;
    }
    return false;
}

const bool TextViewer::keyHold()
{
    switch (m_lastPressed) {
        case MYKEY_UP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_UP])) return moveUp(1);
            break;
        case MYKEY_DOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_DOWN])) return moveDown(1);
            break;
        case MYKEY_PAGEUP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_PAGEUP]))
                return moveUp(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_PAGEDOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_PAGEDOWN]))
                return moveDown(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_LEFT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_LEFT])) return moveLeft();
            break;
        case MYKEY_RIGHT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_RIGHT])) return moveRight();
            break;
        default: break;
    }
    return false;
}

bool TextViewer::mouseDown(int button, int x, int y)
{
    switch (button) {
        case SDL_BUTTON_WHEELUP: return moveUp(/*step=*/1);
        case SDL_BUTTON_WHEELDOWN: return moveDown(/*step=*/1);
        case SDL_BUTTON_RIGHT:
        case SDL_BUTTON_X2: m_retVal = -1; return true;
    }
    return false;
}

bool TextViewer::moveUp(unsigned step)
{
    if (first_line_ > 0) {
        if (first_line_ > step) {
            first_line_ -= step;
        } else {
            first_line_ = 0;
        }
        return true;
    }
    return false;
}

bool TextViewer::moveDown(unsigned step)
{
    if (first_line_ + VIEWER_NB_LINES + 1 < lines_.size()) {
        if (first_line_ + VIEWER_NB_LINES + 1 + step > lines_.size()) {
            first_line_ = lines_.size() - VIEWER_NB_LINES - 1;
        } else {
            first_line_ += step;
        }
        return true;
    }
    return false;
}

bool TextViewer::moveLeft()
{
    if (clip_.x > 0) {
        if (clip_.x > VIEWER_X_STEP * screen.ppu_x) {
            clip_.x -= VIEWER_X_STEP * screen.ppu_x;
        } else {
            clip_.x = 0;
        }
        return true;
    }
    return false;
}

bool TextViewer::moveRight()
{
    clip_.x += VIEWER_X_STEP * screen.ppu_x;
    return true;
}
