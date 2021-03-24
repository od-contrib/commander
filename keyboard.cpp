#include "keyboard.h"

#include <array>
#include <iostream>
#include <tuple>

#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

namespace
{

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

const KeyboardLayout &UsAsciiLayout()
{
    static const auto *const kLayout = new KeyboardLayout {
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
                    kKeycapSpace, kKeycapBackspace },
            },
        },
        /*max_keys_per_row=*/12,
        /*max_rows=*/4,
    };
    return *kLayout;
}

void removeBorder(SDL_Rect *rect, std::size_t border_width)
{
    rect->x += border_width;
    rect->y += border_width;
    rect->w -= 2 * border_width;
    rect->h -= 2 * border_width;
}

void renderRectWithBorder(SDL_Surface *out, SDL_Rect rect,
    std::size_t border_width, Uint32 border_color, Uint32 bg_color)
{
    SDL_Rect line = rect;
    line.w = border_width;
    SDL_FillRect(out, &line, border_color);
    line.x = rect.x + rect.w - border_width;
    SDL_FillRect(out, &line, border_color);
    line.x = rect.x;
    line.w = rect.w;
    line.h = border_width;
    SDL_FillRect(out, &line, border_color);
    line.y = rect.y + rect.h - border_width;
    SDL_FillRect(out, &line, border_color);

    removeBorder(&rect, border_width);
    SDL_FillRect(out, &rect, bg_color);
}

std::vector<SDL_Surface *> AllocSurfaces(
    std::size_t len, int w, int h, std::uint32_t color)
{
    std::vector<SDL_Surface *> surfaces;
    surfaces.reserve(len);
    for (std::size_t i = 0; i < len; ++i)
        surfaces.push_back(SDL_utils::createImage(w, h, color));
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

CKeyboard::CKeyboard(const std::string &p_inputText)
    : CWindow()
    , input_text_(p_inputText)
    , m_fonts(CResourceManager::instance().getFonts())
    , footer_(nullptr)
    , input_text_surface_(nullptr)
    , cancel_highlighted_(nullptr)
    , ok_highlighted_(nullptr)
{
    loadKeyboard();
    init();
}

void CKeyboard::onResize()
{
    freeResources();
    init();
}

void CKeyboard::init()
{
    frame_padding_x_ = (2 + kFrameBorder) * screen.ppu_x;
    frame_padding_y_ = (2 + kFrameBorder) * screen.ppu_y;

    focus_x_ = focus_y_ = 0;
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

    SDL_Rect text_field_rect;
    text_field_rect.x = frame_padding_x_;
    text_field_rect.y = frame_padding_y_;
    text_field_rect.w = keyboard_.width;
    text_field_rect.h = kTextFieldHeight * screen.ppu_y;
    text_field_inner_rect_ = text_field_rect;
    removeBorder(&text_field_inner_rect_, kTextFieldBorder);

    kb_buttons_rect_.x = frame_padding_x_;
    kb_buttons_rect_.y = text_field_rect.y + text_field_rect.h
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
    if (keyboard_.collapse_borders)
    {
        ok_rect_.x -= keyboard_.border_w;
        ok_rect_.w += keyboard_.border_w;
    }
    // Possibly slightly extend Cancel/OK for pixel-perfect alignment.
    const int extend_by
        = keyboard_.width - (ok_rect_.x + ok_rect_.w - cancel_rect_.x);
    cancel_rect_.w += extend_by;
    ok_rect_.x += extend_by;

    for (auto &surface : surfaces_)
    {
        // Overall background:
        renderRectWithBorder(surface,
            SDL_Rect { 0, 0, static_cast<decltype(SDL_Rect {}.w)>(width_),
                static_cast<decltype(SDL_Rect {}.h)>(height_) },
            kFrameBorder, border_color_, bg2_color_);

        // Text field:
        renderRectWithBorder(surface, text_field_rect, kTextFieldBorder,
            border_color_, bg_color_);

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

    cancel_highlighted_
        = SDL_utils::createSurface(cancel_rect_.w, cancel_rect_.h);
    renderButtonHighlighted(*cancel_highlighted_,
        SDL_Rect { 0, 0, cancel_rect_.w, cancel_rect_.h }, "Cancel");
    ok_highlighted_ = SDL_utils::createSurface(ok_rect_.w, ok_rect_.h);
    renderButtonHighlighted(
        *ok_highlighted_, SDL_Rect { 0, 0, ok_rect_.w, ok_rect_.h }, "OK");

    footer_ = SDL_utils::createImage(screen.actual_w, FOOTER_H * screen.ppu_y,
        SDL_MapRGB(surfaces_[0]->format, COLOR_TITLE_BG));
    SDL_utils::applyText(screen.w / 2, 1, footer_, m_fonts,
        "A-Input B-Cancel START-OK L/R⇧ Y← X␣", Globals::g_colorTextTitle,
        { COLOR_TITLE_BG }, SDL_utils::T_TEXT_ALIGN_CENTER);

    renderInputText();
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

CKeyboard::~CKeyboard() { freeResources(); }

void CKeyboard::freeResources()
{
    // Free surfaces
    for (auto *surface : { &input_text_surface_, &cancel_highlighted_,
             &ok_highlighted_, &footer_ })
    {
        if (*surface != nullptr)
        {
            SDL_FreeSurface(*surface);
            *surface = nullptr;
        }
    }
    for (auto *surface : surfaces_) SDL_FreeSurface(surface);
    surfaces_.clear();
    for (auto *surface : kb_highlighted_surfaces_) SDL_FreeSurface(surface);
    kb_highlighted_surfaces_.clear();
}

void CKeyboard::loadKeyboard()
{
    keyboard_.layout = UsAsciiLayout();
    keyboard_.current_keyset = 0;
}

std::pair<int, int> CKeyboard::getKeyCoordinates(int x, int y) const
{
    const auto &kb = keyboard_;
    std::pair<int, int> result {
        x * (kb.key_w + kb.key_gap * screen.ppu_x),
        y * (kb.key_h + kb.key_gap * screen.ppu_y),
    };
    if (kb.collapse_borders)
    {
        result.first -= x * kb.border_w;
        result.second -= y * kb.border_w;
    }
    return result;
}

void CKeyboard::calculateKeyboardDimensions(std::size_t max_w)
{
    auto &kb = keyboard_;
    const std::size_t w = std::min(static_cast<std::size_t>(max_w / screen.ppu_x
                                       / kb.layout.max_keys_per_row),
                              kMaxKeyW + kMaxKeyGap * 2)
        / 2 * 2;
    if (w > kMaxKeyW)
        kb.key_gap = std::min(
            static_cast<std::size_t>((w * 0.2) / 4) * 2, kMaxKeyGap);
    else
        kb.key_gap = 0;
    kb.collapse_borders = kb.key_gap > 0 ? 0 : 1;
    kb.key_w = (w - 2 * kb.key_gap) * screen.ppu_x;
    kb.key_h = std::min(kb.key_w, kMaxKeyH) * screen.ppu_y;
    keycap_text_offset_y_
        = std::max(0, std::min(static_cast<int>(kMaxKeyH) - 15, 3))
        * screen.ppu_y;
    kb.border_w = kKeyBorderW;
    std::tie(kb.width, kb.height)
        = getKeyCoordinates(kb.layout.max_keys_per_row, kb.layout.max_rows);
    kb.width -= kb.key_gap * screen.ppu_x;
    kb.height -= kb.key_gap * screen.ppu_y;
    if (kb.collapse_borders)
    {
        kb.width += kb.border_w;
        kb.height += kb.border_w;
    }
}

void CKeyboard::renderKeys(std::vector<SDL_Surface *> &out_surfaces, Sint16 x0,
    Sint16 y0, std::uint32_t key_bg_color, SDL_Color sdl_key_bg_color,
    std::uint32_t key_border_color) const
{
    const auto &kb = keyboard_;
    SDL_Rect key_rect;
    key_rect.w = kb.key_w;
    key_rect.h = kb.key_h;
    auto surface_it = out_surfaces.begin();
    for (const auto &layer : kb.layout.layers)
    {
        auto *out = *surface_it++;
        key_rect.y = y0;
        for (const auto &row : layer)
        {
            key_rect.x = x0;
            for (const auto &key : row)
            {
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

void CKeyboard::renderInputText()
{
    constexpr std::size_t kTextOffsetX = 5;
    constexpr std::size_t kTextOffsetY = 3;
    if (input_text_surface_ == nullptr)
    {
        input_text_surface_ = SDL_utils::createSurface(
            text_field_inner_rect_.w, text_field_inner_rect_.h);
    }
    const auto bg_color = SDL_MapRGB(input_text_surface_->format, COLOR_BG_1);
    SDL_FillRect(input_text_surface_, nullptr, bg_color);
    if (!input_text_.empty())
    {
        SDL_Surface *tmp_surface = SDL_utils::renderText(
            m_fonts, input_text_, Globals::g_colorTextNormal, { COLOR_BG_1 });
        if (tmp_surface->w > text_field_inner_rect_.w)
        {
            // Text is too big => clip it
            SDL_Rect rect;
            rect.x = tmp_surface->w - text_field_inner_rect_.w;
            rect.y = 0;
            rect.w = text_field_inner_rect_.w;
            rect.h = tmp_surface->h;
            SDL_utils::applySurface(kTextOffsetX, kTextOffsetY, tmp_surface,
                input_text_surface_, &rect);
        }
        else
        {
            SDL_utils::applySurface(
                kTextOffsetX, kTextOffsetY, tmp_surface, input_text_surface_);
        }
        SDL_FreeSurface(tmp_surface);
    }
}

void CKeyboard::render(const bool p_focus) const
{
    INHIBIT(std::cout << "CKeyboard::render  fullscreen: " << isFullScreen()
                      << "  focus: " << p_focus << std::endl;)
    // Draw background layer
    SDL_utils::applyPpuScaledSurface(
        x_, y_, surfaces_[keyboard_.current_keyset], screen.surface);

    // Draw input text
    SDL_utils::applyPpuScaledSurface(x_ + text_field_inner_rect_.x,
        y_ + text_field_inner_rect_.y, input_text_surface_, screen.surface);

    // Draw focused button
    if (isFocusOnButtonsRow())
    {
        if (isFocusOnCancel())
            SDL_utils::applyPpuScaledSurface(x_ + cancel_rect_.x,
                y_ + cancel_rect_.y, cancel_highlighted_, screen.surface);
        else
            SDL_utils::applyPpuScaledSurface(x_ + ok_rect_.x, y_ + ok_rect_.y,
                ok_highlighted_, screen.surface);
    }
    else
    {
        SDL_Rect clip_rect;
        std::tie(clip_rect.x, clip_rect.y)
            = getKeyCoordinates(focus_x_, focus_y_);
        clip_rect.w = keyboard_.key_w;
        clip_rect.h = keyboard_.key_h;
        SDL_utils::applyPpuScaledSurface(x_ + kb_buttons_rect_.x + clip_rect.x,
            y_ + kb_buttons_rect_.y + clip_rect.y,
            kb_highlighted_surfaces_[keyboard_.current_keyset], screen.surface,
            &clip_rect);
    }
    // Draw footer
    SDL_utils::applySurface(0, screen.h - FOOTER_H, footer_, screen.surface);
}

const bool CKeyboard::keyPress(const SDL_Event &p_event)
{
    CWindow::keyPress(p_event);
    bool l_ret(false);
    switch (p_event.key.keysym.sym)
    {
        case MYKEY_PARENT:
            // B => Cancel
            m_retVal = -1;
            l_ret = true;
            break;
        case MYKEY_UP: l_ret = moveCursorUp(true); break;
        case MYKEY_DOWN: l_ret = moveCursorDown(true); break;
        case MYKEY_LEFT: l_ret = moveCursorLeft(true); break;
        case MYKEY_RIGHT: l_ret = moveCursorRight(true); break;
        case MYKEY_SYSTEM:
            // Y => Backspace
            l_ret = backspace();
            break;
        case MYKEY_OPERATION:
            // X => Space
            l_ret = appendText(" ");
            break;
        case MYKEY_OPEN:
            // A => press button
            l_ret = pressFocusedKey();
            break;
        case MYKEY_PAGEDOWN:
            // R => Change keys forward
            keyboard_.current_keyset
                = (keyboard_.current_keyset + 1) % keyboard_.num_keysets();
            l_ret = true;
            break;
        case MYKEY_PAGEUP:
            // L => Change keys backward
            keyboard_.current_keyset
                = (keyboard_.num_keysets() + keyboard_.current_keyset - 1)
                % keyboard_.num_keysets();
            l_ret = true;
            break;
        case MYKEY_TRANSFER:
            // START => OK
            m_retVal = 1;
            l_ret = true;
        default: break;
    }
    return l_ret;
}

const bool CKeyboard::keyHold(void)
{
    bool l_ret(false);
    switch (m_lastPressed)
    {
        case MYKEY_UP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_UP]))
                l_ret = moveCursorUp(false);
            break;
        case MYKEY_DOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_DOWN]))
                l_ret = moveCursorDown(false);
            break;
        case MYKEY_LEFT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_LEFT]))
                l_ret = moveCursorLeft(false);
            break;
        case MYKEY_RIGHT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_RIGHT]))
                l_ret = moveCursorRight(false);
            break;
        case MYKEY_OPEN:
            // A => Add letter
            if (tick(SDL_GetKeyState(NULL)[MYKEY_OPEN]))
                l_ret = pressFocusedKey();
            break;
        case MYKEY_SYSTEM:
            // Y => Backspace
            if (tick(SDL_GetKeyState(NULL)[MYKEY_SYSTEM])) l_ret = backspace();
            break;
        case MYKEY_OPERATION:
            // X => Space
            if (tick(SDL_GetKeyState(NULL)[MYKEY_OPERATION]))
                l_ret = appendText(" ");
            break;
        default: break;
    }
    return l_ret;
}

const bool CKeyboard::moveCursorUp(const bool p_loop)
{
    if (!p_loop && focus_y_ == 0) return false;

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

const bool CKeyboard::isFocusOnButtonsRow() const
{
    return focus_y_ == keyboard_.num_rows();
}

const bool CKeyboard::isFocusOnOk() const
{
    return isFocusOnButtonsRow() && focus_x_ == 1;
}

const bool CKeyboard::isFocusOnCancel() const
{
    return isFocusOnButtonsRow() && focus_x_ == 0;
}

bool CKeyboard::pressFocusedKey()
{
    bool updated = false;
    if (isFocusOnButtonsRow()) { m_retVal = isFocusOnCancel() ? -1 : 1; }
    else if (keyboard_.isBackspace(focus_x_, focus_y_))
    {
        updated = backspace();
    }
    else
    {
        updated = true;
        input_text_.append(keyboard_.text(focus_x_, focus_y_));
    }
    if (updated) renderInputText();
    return true;
}

bool CKeyboard::appendText(const std::string &text)
{
    input_text_.append(text);
    renderInputText();
    return true;
}

const std::string &CKeyboard::getInputText(void) const { return input_text_; }

const bool CKeyboard::backspace(void)
{
    bool l_ret(false);
    if (!input_text_.empty())
    {
        if (input_text_.size() >= 2
            && utf8Code(input_text_.at(input_text_.size() - 2)))
            input_text_.resize(input_text_.size() - 2);
        else
            input_text_.resize(input_text_.size() - 1);
        l_ret = true;
    }
    return l_ret;
}

const bool CKeyboard::utf8Code(const unsigned char p_c) const
{
    return (p_c >= 194 && p_c <= 198) || p_c == 208 || p_c == 209;
}
