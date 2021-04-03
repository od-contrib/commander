#include "keyboard.h"

#include <array>
#include <cctype>
#include <iostream>
#include <tuple>

#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

#ifdef KEYBOARD_SWAP_SYSTEM_AND_PARENT
#define KEYBOARD_KEY_BACKSPACE MYKEY_SYSTEM
#define KEYBOARD_KEY_CANCEL MYKEY_PARENT
#else
#define KEYBOARD_KEY_BACKSPACE MYKEY_PARENT
#define KEYBOARD_KEY_CANCEL MYKEY_SYSTEM
#endif
namespace {

using SDL_utils::removeBorder;
using SDL_utils::renderRectWithBorder;

constexpr char kKeycapBackspace[] = "←";
constexpr char kKeycapSpace[] = " ";
constexpr char kKeycapTab[] = "⇥";

static std::string kSpace = " ";
static std::string kTab = "\t";

constexpr std::size_t kKeyBorderW = 1;
constexpr std::size_t kMaxKeyW = 19 + kKeyBorderW;
constexpr std::size_t kMaxKeyH = 17 + kKeyBorderW;
constexpr std::size_t kMaxKeyGap = 2;
constexpr std::size_t kFrameBorder = 2;
constexpr std::size_t kTextFieldHeight = 19;
constexpr std::size_t kTextFieldMarginBottom = 2;
constexpr std::size_t kTextFieldBorder = 2;

KeyboardLayout UsAsciiLayout(bool support_tabs)
{
    return KeyboardLayout {
        {
            {
                { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=" },
                { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]" },
                { "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "\\" },
                { "z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
                    kKeycapSpace, kKeycapBackspace },
            },
            {
                { "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+" },
                { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}" },
                { "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "|" },
                { "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?",
                    support_tabs ? kKeycapTab : kKeycapSpace,
                    kKeycapBackspace },
            },
        },
        /*max_keys_per_row=*/12,
        /*max_rows=*/4,
    };
}

std::vector<SDLSurfaceUniquePtr> AllocSurfaces(
    std::size_t len, int w, int h, std::uint32_t color)
{
    std::vector<SDLSurfaceUniquePtr> surfaces;
    surfaces.reserve(len);
    for (std::size_t i = 0; i < len; ++i)
        surfaces.emplace_back(SDL_utils::createImage(w, h, color));
    return surfaces;
}

} // namespace

const std::string &CKeyboard::Keyboard::keycap(
    std::size_t x, std::size_t y) const
{
    return current_keys()[y][x];
}

bool CKeyboard::Keyboard::isBackspace(std::size_t x, std::size_t y) const
{
    return keycap(x, y) == kKeycapBackspace;
}

const std::string &CKeyboard::Keyboard::text(std::size_t x, std::size_t y) const
{
    const std::string &str = keycap(x, y);
    if (str == kKeycapSpace) return kSpace;
    if (str == kKeycapTab) return kTab;
    return str;
}

CKeyboard::CKeyboard(const std::string &p_inputText, bool support_tabs)
    : CWindow()
    , m_fonts(CResourceManager::instance().getFonts())
    , support_tabs_(support_tabs)
    , text_edit_(support_tabs)
{
    text_edit_.typeText(p_inputText);
    loadKeyboard();
    init();
}

void CKeyboard::onResize() { init(); }

void CKeyboard::init()
{
    frame_padding_x_ = (2 + kFrameBorder) * screen.ppu_x;
    frame_padding_y_ = (2 + kFrameBorder) * screen.ppu_y;

    focusOnTextEdit();
    calculateKeyboardDimensions(screen.actual_w - 2);

    const int ok_cancel_height = keyboard_.key_h;

    width_ = 2 * frame_padding_x_ + keyboard_.width;
    height_ = 2 * frame_padding_y_ + keyboard_.height + ok_cancel_height
        + (kTextFieldHeight + kTextFieldMarginBottom + keyboard_.key_gap)
            * screen.ppu_y;
    x_ = (screen.actual_w - width_) / 2;
    y_ = screen.actual_h - (height_ + (8 + FOOTER_H) * screen.ppu_y);

    surfaces_ = AllocSurfaces(keyboard_.layout.layers.size(), width_, height_,
        SDL_MapRGB(screen.surface->format, COLOR_BG_1));

    const SDL_PixelFormat *pixel_format = surfaces_[0]->format;
    border_color_ = SDL_MapRGB(pixel_format, COLOR_BORDER);
    sdl_bg_color_ = SDL_Color { COLOR_BG_1 };
    bg_color_ = SDL_MapRGB(pixel_format, COLOR_BG_1);
    bg2_color_ = SDL_MapRGB(pixel_format, COLOR_BG_2);
    sdl_highlight_color_ = SDL_Color { COLOR_CURSOR_1 };
    highlight_color_ = SDL_MapRGB(pixel_format, COLOR_CURSOR_1);

    text_field_rect_.x = frame_padding_x_;
    text_field_rect_.y = frame_padding_y_;
    text_field_rect_.w = keyboard_.width;
    text_field_rect_.h = kTextFieldHeight * screen.ppu_y;
    text_edit_.setDimensions(text_field_rect_.w, text_field_rect_.h);

    kb_buttons_rect_.x = frame_padding_x_;
    kb_buttons_rect_.y = text_field_rect_.y + text_field_rect_.h
        + kTextFieldMarginBottom * screen.ppu_y;
    kb_buttons_rect_.w = width_ - 2 * frame_padding_x_;
    kb_buttons_rect_.h = keyboard_.height + ok_cancel_height
        + keyboard_.key_gap * screen.ppu_y;

    kb_highlighted_surfaces_
        = AllocSurfaces(keyboard_.layout.layers.size(), kb_buttons_rect_.w,
            kb_buttons_rect_.h, SDL_MapRGB(screen.surface->format, COLOR_BG_1));

    cancel_rect_.x = kb_buttons_rect_.x;
    cancel_rect_.y = kb_buttons_rect_.y + kb_buttons_rect_.h - ok_cancel_height;
    if (keyboard_.collapse_borders) cancel_rect_.y -= keyboard_.border_w;

    cancel_rect_.w = (keyboard_.width - keyboard_.key_gap * screen.ppu_x) / 2;
    cancel_rect_.h = ok_cancel_height;

    ok_rect_ = cancel_rect_;
    ok_rect_.x
        = cancel_rect_.x + cancel_rect_.w + keyboard_.key_gap * screen.ppu_x;
    if (keyboard_.collapse_borders) {
        ok_rect_.x -= keyboard_.border_w;
        ok_rect_.w += keyboard_.border_w;
    }
    // Possibly slightly extend Cancel/OK for pixel-perfect alignment.
    const int extend_by
        = keyboard_.width - (ok_rect_.x + ok_rect_.w - cancel_rect_.x);
    cancel_rect_.w += extend_by;
    ok_rect_.x += extend_by;

    for (auto &surface : surfaces_) {
        // Overall background:
        renderRectWithBorder(surface.get(),
            SDL_Rect { 0, 0, static_cast<decltype(SDL_Rect {}.w)>(width_),
                static_cast<decltype(SDL_Rect {}.h)>(height_) },
            kFrameBorder, border_color_, bg2_color_);

        text_edit_.blitBackground(
            *surface, text_field_rect_.x, text_field_rect_.y);

        // OK / Cancel buttons:
        renderButton(*surface, cancel_rect_, "Cancel");
        renderButton(*surface, ok_rect_, "OK");
    }

    renderKeys(surfaces_, kb_buttons_rect_.x, kb_buttons_rect_.y,
        /*key_bg_color=*/bg_color_, sdl_bg_color_,
        /*key_border_color=*/border_color_);
    renderKeys(kb_highlighted_surfaces_, 0, 0,
        /*key_bg_color=*/highlight_color_, sdl_highlight_color_,
        /*key_border_color=*/border_color_);

    cancel_highlighted_.reset(
        SDL_utils::createSurface(cancel_rect_.w, cancel_rect_.h));
    renderButtonHighlighted(*cancel_highlighted_,
        SDL_Rect { 0, 0, cancel_rect_.w, cancel_rect_.h }, "Cancel");
    ok_highlighted_.reset(SDL_utils::createSurface(ok_rect_.w, ok_rect_.h));
    renderButtonHighlighted(
        *ok_highlighted_, SDL_Rect { 0, 0, ok_rect_.w, ok_rect_.h }, "OK");

    footer_.reset(
        SDL_utils::createImage(screen.actual_w, FOOTER_H * screen.ppu_y,
            SDL_MapRGB(surfaces_[0]->format, COLOR_TITLE_BG)));
    SDL_utils::applyText(screen.w / 2, 1, footer_.get(), m_fonts,
        "A-Input B-Cancel START-OK L/R⇧ Y← X␣", Globals::g_colorTextTitle,
        { COLOR_TITLE_BG }, SDL_utils::T_TEXT_ALIGN_CENTER);
}

void CKeyboard::renderButton(
    SDL_Surface &out, SDL_Rect rect, const std::string &text) const
{
    return renderButton(
        out, rect, text, border_color_, bg_color_, sdl_bg_color_);
}

void CKeyboard::renderButtonHighlighted(
    SDL_Surface &out, SDL_Rect rect, const std::string &text) const
{
    return renderButton(
        out, rect, text, border_color_, highlight_color_, sdl_highlight_color_);
}

void CKeyboard::renderButton(SDL_Surface &out, SDL_Rect rect,
    const std::string &text, std::uint32_t border_color, std::uint32_t bg_color,
    SDL_Color sdl_bg_color) const
{
    renderRectWithBorder(&out, rect, 1, border_color, bg_color);
    SDL_utils::applyPpuScaledText(rect.x + rect.w / 2,
        rect.y + keycap_text_offset_y_, &out, m_fonts, text,
        Globals::g_colorTextNormal, sdl_bg_color,
        SDL_utils::T_TEXT_ALIGN_CENTER);
}

void CKeyboard::loadKeyboard()
{
    keyboard_.layout = UsAsciiLayout(support_tabs_);
    keyboard_.current_keyset = 0;
}

SDL_Point CKeyboard::getKeyCoordinates(int x, int y) const
{
    const auto &kb = keyboard_;
    SDL_Point result;
    result.x = x * (kb.key_w + kb.key_gap * screen.ppu_x);
    result.y = y * (kb.key_h + kb.key_gap * screen.ppu_y);
    if (kb.collapse_borders) {
        result.x -= x * kb.border_w;
        result.y -= y * kb.border_w;
    }
    return result;
}

std::pair<int, int> CKeyboard::getButtonAt(SDL_Point p) const
{
    const auto &kb = keyboard_;
    const int x_gap = kb.key_gap * screen.ppu_x;
    const int y_gap = kb.key_gap * screen.ppu_y;
    SDL_Rect buttons_rect = kb_buttons_rect_;
    buttons_rect.x += x_;
    buttons_rect.y += y_;
    buttons_rect.h += kb.key_h; // OK / Cancel row
    if (!SDL_PointInRect(&p, &buttons_rect)) return { -1, -1 };

    const int y_idx = (p.y - buttons_rect.y)
        / (kb.key_h + y_gap - (kb.collapse_borders ? kb.border_w : 0));
    if (y_idx == kb.num_rows()) {
        // Buttons row. We cheat slightly here and ignore the gap.
        const int x_idx = 2 * (p.x - buttons_rect.x) / buttons_rect.w;
        return { x_idx, y_idx };
    }
    const int x_idx = (p.x - buttons_rect.x)
        / (kb.key_w + x_gap - (kb.collapse_borders ? kb.border_w : 0));

    // Check that we're not in a gap:
    const SDL_Point p_top_left = getKeyCoordinates(x_idx, y_idx);
    if (p.x - buttons_rect.x - p_top_left.x > kb.key_w
        || p.y - buttons_rect.y - p_top_left.y > kb.key_h)
        return { -1, -1 };

    return { x_idx, y_idx };
}

void CKeyboard::calculateKeyboardDimensions(std::size_t max_w)
{
    auto &kb = keyboard_;
    const std::size_t w = std::min(static_cast<std::size_t>(max_w / screen.ppu_x
                                       / kb.layout.max_keys_per_row),
                              kMaxKeyW + kMaxKeyGap * 2)
        / 2 * 2;
    if (w > kMaxKeyW)
        kb.key_gap
            = std::min(static_cast<std::size_t>((w * 0.2) / 4) * 2, kMaxKeyGap);
    else
        kb.key_gap = 0;
    kb.collapse_borders = kb.key_gap > 0 ? 0 : 1;
    kb.key_w = (w - 2 * kb.key_gap) * screen.ppu_x;
    kb.key_h = std::min(kb.key_w, kMaxKeyH) * screen.ppu_y;
    keycap_text_offset_y_
        = std::max(0, std::min(static_cast<int>(kMaxKeyH) - 15, 3))
        * screen.ppu_y;
    kb.border_w = kKeyBorderW;
    const SDL_Point end
        = getKeyCoordinates(kb.layout.max_keys_per_row, kb.layout.max_rows);
    kb.width = end.x - kb.key_gap * screen.ppu_x;
    kb.height = end.y - kb.key_gap * screen.ppu_y;
    if (kb.collapse_borders) {
        kb.width += kb.border_w;
        kb.height += kb.border_w;
    }
}

void CKeyboard::renderKeys(std::vector<SDLSurfaceUniquePtr> &out_surfaces,
    Sint16 x0, Sint16 y0, std::uint32_t key_bg_color,
    SDL_Color sdl_key_bg_color, std::uint32_t key_border_color) const
{
    const auto &kb = keyboard_;
    SDL_Rect key_rect;
    key_rect.w = kb.key_w;
    key_rect.h = kb.key_h;
    auto surface_it = out_surfaces.begin();
    for (const auto &layer : kb.layout.layers) {
        auto *out = surface_it->get();
        ++surface_it;
        key_rect.y = y0;
        for (const auto &row : layer) {
            key_rect.x = x0;
            for (const auto &key : row) {
                renderRectWithBorder(
                    out, key_rect, kb.border_w, key_border_color, key_bg_color);
                SDL_utils::applyPpuScaledText(
                    key_rect.x + kb.border_w + kb.key_w / 2,
                    key_rect.y + kb.border_w + keycap_text_offset_y_, out,
                    m_fonts, key, Globals::g_colorTextNormal, sdl_key_bg_color,
                    SDL_utils::T_TEXT_ALIGN_CENTER);
                key_rect.x += kb.key_w + kb.key_gap * screen.ppu_x
                    - (kb.collapse_borders ? kb.border_w : 0);
            }
            key_rect.y += kb.key_h + kb.key_gap * screen.ppu_y
                - (kb.collapse_borders ? kb.border_w : 0);
        }
    }
}

void CKeyboard::render(const bool p_focus) const
{
    INHIBIT(std::cout << "CKeyboard::render  fullscreen: " << isFullScreen()
                      << "  focus: " << p_focus << std::endl;)
    // Draw background layer
    SDL_utils::applyPpuScaledSurface(
        x_, y_, surfaces_[keyboard_.current_keyset].get(), screen.surface);

    // Draw input text
    text_edit_.blitForeground(
        *screen.surface, x_ + text_field_rect_.x, y_ + text_field_rect_.y);

    // Draw focused button
    if (isFocusOnButtonsRow()) {
        if (isFocusOnCancel())
            SDL_utils::applyPpuScaledSurface(x_ + cancel_rect_.x,
                y_ + cancel_rect_.y, cancel_highlighted_.get(), screen.surface);
        else
            SDL_utils::applyPpuScaledSurface(x_ + ok_rect_.x, y_ + ok_rect_.y,
                ok_highlighted_.get(), screen.surface);
    } else {
        SDL_Rect clip_rect;
        const SDL_Point key_top_left = getKeyCoordinates(focus_x_, focus_y_);
        clip_rect.x = key_top_left.x;
        clip_rect.y = key_top_left.y;
        clip_rect.w = keyboard_.key_w;
        clip_rect.h = keyboard_.key_h;
        SDL_utils::applyPpuScaledSurface(x_ + kb_buttons_rect_.x + clip_rect.x,
            y_ + kb_buttons_rect_.y + clip_rect.y,
            kb_highlighted_surfaces_[keyboard_.current_keyset].get(),
            screen.surface, &clip_rect);
    }
    // Draw footer
    SDL_utils::applyPpuScaledSurface(0,
        screen.actual_h - FOOTER_H * screen.ppu_y, footer_.get(),
        screen.surface);
}

const bool CKeyboard::keyPress(const SDL_Event &p_event)
{
    CWindow::keyPress(p_event);
    const auto keysym = p_event.key.keysym;
    switch (keysym.sym) {
        case KEYBOARD_KEY_CANCEL:
            // B => Cancel
            m_retVal = -1;
            return true;
        case KEYBOARD_KEY_BACKSPACE:
            // Y => Backspace
            return text_edit_.backspace();
        case MYKEY_UP: return moveCursorUp(true);
        case MYKEY_DOWN: return moveCursorDown(true);
        case MYKEY_LEFT:
            return isFocusOnTextEdit() ? text_edit_.moveCursorPrev()
                                       : moveCursorLeft(true);
        case MYKEY_RIGHT:
            return isFocusOnTextEdit() ? text_edit_.moveCursorNext()
                                       : moveCursorRight(true);
        case MYKEY_OPERATION:
            // X => Space
            text_edit_.typeText(' ');
            return true;
        case MYKEY_OPEN:
            // A => press button
            return pressFocusedKey();
        case MYKEY_PAGEDOWN:
            // R => Change keys forward
            keyboard_.current_keyset
                = (keyboard_.current_keyset + 1) % keyboard_.num_keysets();
            return true;
        case MYKEY_PAGEUP:
            // L => Change keys backward
            keyboard_.current_keyset
                = (keyboard_.num_keysets() + keyboard_.current_keyset - 1)
                % keyboard_.num_keysets();
            return true;
        case MYKEY_TRANSFER:
            // START => OK
            m_retVal = 1;
            return true;
        default:
            switch (keysym.sym) {
                case SDLK_BACKSPACE:
                case SDLK_DELETE: return text_edit_.backspace();
                case SDLK_HOME:
                    return text_edit_.setCursorToStart();
                case SDLK_END:
                    return text_edit_.setCursorToEnd();
                default:
#ifndef USE_SDL2
                    if ((keysym.unicode & 0xFF80) == 0) {
                        const unsigned char c = keysym.unicode & 0x7F;
                        if (std::isprint(c)) {
                            text_edit_.typeText(c);
                            return true;
                        }
                    }
#endif
                    return false;
            }
    }
}

const bool CKeyboard::keyHold(void)
{
    switch (m_lastPressed) {
        case MYKEY_UP:
            if (tick(MYKEY_UP))
                return moveCursorUp(false);
            return false;
        case MYKEY_DOWN:
            if (tick(MYKEY_DOWN))
                return moveCursorDown(false);
            return false;
        case MYKEY_LEFT:
            if (tick(MYKEY_LEFT))
                return isFocusOnTextEdit() ? text_edit_.moveCursorPrev()
                                           : moveCursorLeft(false);
            return false;
        case MYKEY_RIGHT:
            if (tick(MYKEY_RIGHT))
                return isFocusOnTextEdit() ? text_edit_.moveCursorNext()
                                           : moveCursorRight(false);
            return false;
        case MYKEY_OPEN:
            // A => Add letter
            if (tick(MYKEY_OPEN))
                return pressFocusedKey();
            return false;
        case MYKEY_SYSTEM:
            // Y => Backspace
            if (tick(MYKEY_SYSTEM))
                return text_edit_.backspace();
            return false;
        case MYKEY_OPERATION:
            // X => Space
            if (tick(MYKEY_OPERATION)) {
                text_edit_.typeText(' ');
                return true;
            }
            return false;
        default: return false;
    }
    return true;
}

bool CKeyboard::mouseDown(int button, int x, int y)
{
    if (x < x_ || x > x_ + width_ || y < y_ || y > y_ + height_) {
        m_retVal = -1;
        return true;
    }
    const std::pair<int, int> key = getButtonAt(SDL_Point { x, y });
    if (key.first == -1) return false;
    switch (button) {
        case SDL_BUTTON_LEFT:
            focus_x_ = key.first;
            focus_y_ = key.second;
            pressFocusedKey();
            return true;
        case SDL_BUTTON_MIDDLE:
        case SDL_BUTTON_RIGHT:
            focus_x_ = key.first;
            focus_y_ = key.second;
            return true;
        case SDL_BUTTON_X2: m_retVal = -1; return true;
    }
    return false;
}

const bool CKeyboard::moveCursorUp(const bool p_loop)
{
    if (!p_loop && focus_y_ == -1) return false;

    // Special case: going to/from text edit.
    if (focus_y_ == 0) {
        focusOnTextEdit();
        return true;
    } else if (isFocusOnTextEdit()) {
        text_edit_.setFocused(false);
        focus_y_ = keyboard_.num_rows();
        focus_x_ = 0;
        return true;
    }

    // Special case: going from buttons to keyboard:
    if (isFocusOnButtonsRow())
        focus_x_ += (keyboard_.num_row_keys(focus_y_ - 1) - 1) / 2;

    const std::size_t height = keyboard_.num_rows() + 1;
    focus_y_ = (height + focus_y_ - 1) % height;
    return true;
}

const bool CKeyboard::moveCursorDown(const bool p_loop)
{
    if (!p_loop && isFocusOnButtonsRow()) return false;

    // Special case: going to/from text edit.
    if (isFocusOnButtonsRow()) {
        focusOnTextEdit();
        return true;
    } else if (isFocusOnTextEdit()) {
        text_edit_.setFocused(false);
        focus_y_ = 0;
        focus_x_ = 0;
        return true;
    }

    // Special case: going from keyboard to buttons:
    if (focus_y_ == keyboard_.num_rows() - 1)
        focus_x_ = (focus_x_ >= keyboard_.num_row_keys(focus_y_) / 2) ? 1 : 0;

    const std::size_t height = keyboard_.num_rows() + 1;
    focus_y_ = (focus_y_ + 1) % height;
    return true;
}

const bool CKeyboard::moveCursorLeft(const bool p_loop)
{
    if (!p_loop && focus_x_ == 0) return false;
    const std::size_t width
        = isFocusOnButtonsRow() ? 2 : keyboard_.current_keys()[focus_y_].size();
    focus_x_ = (width + focus_x_ - 1) % width;
    return true;
}

const bool CKeyboard::moveCursorRight(const bool p_loop)
{
    const std::size_t width
        = isFocusOnButtonsRow() ? 2 : keyboard_.current_keys()[focus_y_].size();
    if (!p_loop && focus_x_ + 1 == width) return false;
    focus_x_ = (focus_x_ + 1) % width;
    return true;
}

void CKeyboard::focusOnTextEdit()
{
    text_edit_.setFocused(true);
    focus_y_ = -1;
    focus_x_ = 0;
}

bool CKeyboard::isFocusOnTextEdit() const
{
    return focus_y_ == -1;
}

bool CKeyboard::isFocusOnButtonsRow() const
{
    return focus_y_ == keyboard_.num_rows();
}

bool CKeyboard::isFocusOnOk() const
{
    return isFocusOnButtonsRow() && focus_x_ == 1;
}

bool CKeyboard::isFocusOnCancel() const
{
    return isFocusOnButtonsRow() && focus_x_ == 0;
}

bool CKeyboard::pressFocusedKey()
{
    bool updated = false;
    if (isFocusOnButtonsRow()) {
        m_retVal = isFocusOnCancel() ? -1 : 1;
    } else if (keyboard_.isBackspace(focus_x_, focus_y_)) {
        updated = text_edit_.backspace();
    } else {
        text_edit_.typeText(keyboard_.text(focus_x_, focus_y_));
        updated = true;
    }
    return true;
}

const std::string &CKeyboard::getInputText(void) const
{
    return text_edit_.text();
}
