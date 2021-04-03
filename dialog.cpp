#include <iostream>
#include "dialog.h"
#include "screen.h"
#include "sdlutils.h"
#include "resourceManager.h"
#include "def.h"

CDialog::CDialog(const std::string &p_title, std::function<Sint16()> x_fn,
        std::function<Sint16()> y_fn):
    CWindow(),
    m_borderColor({COLOR_BORDER}),
    m_nbTitle(false),
    m_nbLabels(0),
    m_nbOptions(0),
    m_titleImg(NULL),
    m_highlightedLine(0),
    m_image(NULL),
    m_cursor1(NULL),
    m_cursor2(NULL),
    m_x_fn(std::move(x_fn)),
    m_y_fn(std::move(y_fn)),
    m_cursorX(0),
    m_cursorY(0),
    m_fonts(CResourceManager::instance().getFonts())
{
    // Title
    if (!p_title.empty())
    {
        m_nbTitle = true;
        m_lines.push_back(p_title);
    }
    // Init clip
    m_clip.x = 0;
    m_clip.y = 0;
    m_clip.w = 0;
    m_clip.h = 0;
}

CDialog::~CDialog(void)
{
    freeResources();
}

void CDialog::freeResources()
{
    for (auto *surface_ptr : { &m_titleImg, &m_image, &m_cursor1, &m_cursor2 })
    {
        if (*surface_ptr == nullptr) continue;
        SDL_FreeSurface(*surface_ptr);
        *surface_ptr = NULL;
    }
    for (auto *surfaces : { &m_linesImg, &m_linesImgCursor1, &m_linesImgCursor2 })
    {
        for (auto *surface_ptr : *surfaces)
            if (surface_ptr != nullptr) SDL_FreeSurface(surface_ptr);
        surfaces->clear();
    }
}

void CDialog::addLabel(const std::string &p_label)
{
    m_lines.push_back(p_label);
    ++m_nbLabels;
}

void CDialog::addOption(const std::string &p_option)
{
    m_lines.push_back(p_option);
    ++m_nbOptions;
}

void CDialog::init(void)
{
    // The width of the window depends on the width of the largest line
    int l_width(0);
    int l_cursorWidth(0);

    // Title
    auto l_it = m_lines.begin();
    if (m_nbTitle) {
        m_titleImg = SDL_utils::renderText(m_fonts, *l_it, Globals::g_colorTextTitle, m_borderColor);
        // m_titleImg is nullptr when text has zero width.
        l_width = m_titleImg != nullptr ? m_titleImg->w : 0;
        ++l_it;
    }

    // Render every line
    const std::size_t num_non_title_lines = m_lines.size() - (m_nbTitle ? 1 : 0);
    m_linesImg.reserve(num_non_title_lines);

    SDL_Color label_bg{COLOR_BG_1};
    if (m_nbOptions > 1) label_bg = SDL_Color{COLOR_BG_2};
    const SDL_Color option_bg{COLOR_BG_1};

    for (int i = 0; i < m_nbLabels; ++i, ++l_it) {
        m_linesImg.push_back(SDL_utils::renderText(m_fonts, *l_it, Globals::g_colorTextNormal, label_bg));
        if (m_linesImg.back() != nullptr && m_linesImg.back()->w > l_width)
            l_width = m_linesImg.back()->w;
    }

    m_linesImgCursor1.reserve(m_nbOptions);
    m_linesImgCursor2.reserve(m_nbOptions);
    for (int i = 0; i < m_nbOptions; ++i, ++l_it) {
        m_linesImg.push_back(SDL_utils::renderText(m_fonts, *l_it, Globals::g_colorTextNormal, option_bg));
        m_linesImgCursor1.push_back(SDL_utils::renderText(m_fonts, *l_it, Globals::g_colorTextNormal, {COLOR_CURSOR_1}));
        m_linesImgCursor2.push_back(SDL_utils::renderText(m_fonts, *l_it, Globals::g_colorTextNormal, {COLOR_CURSOR_2}));
        if (m_linesImg.back() != nullptr && m_linesImg.back()->w > l_width)
            l_width = m_linesImg.back()->w;
    }
    l_width /= screen.ppu_x;
    // Cursor width
    l_cursorWidth = l_width + 2 * DIALOG_MARGIN;
    if (l_cursorWidth > screen.w - 2 * DIALOG_BORDER)
        l_cursorWidth = screen.w - 2 * DIALOG_BORDER;
    // Line clip
    for (auto *img : m_linesImg)
    {
        if (img == nullptr) continue;
        m_clip.h = img->h;
        break;
    }
    m_clip.w = (l_cursorWidth - DIALOG_MARGIN - 1) * screen.ppu_x;
    // Adjust image width
    l_width = l_width + 2 * DIALOG_MARGIN + 2 * DIALOG_BORDER;
    if (l_width > screen.w)
        l_width = screen.w;
    width_ = l_width * screen.ppu_x;

    // Create dialog image
    const int l_height = m_lines.size() * LINE_HEIGHT + 2 * DIALOG_BORDER;
    height_ = l_height * screen.ppu_y;

    m_image = SDL_utils::createImage(l_width * screen.ppu_x,
        l_height * screen.ppu_y,
        SDL_MapRGB(screen.surface->format, m_borderColor.r, m_borderColor.g,
            m_borderColor.b));
    if (m_nbLabels > 0) {
        SDL_Rect rect = SDL_utils::makeRect(DIALOG_BORDER * screen.ppu_x,
            (DIALOG_BORDER + (m_nbTitle ? 1 : 0) * LINE_HEIGHT) * screen.ppu_y,
            m_image->w - 2 * DIALOG_BORDER * screen.ppu_x,
            m_nbLabels * LINE_HEIGHT * screen.ppu_y);
        SDL_FillRect(m_image, &rect, SDL_utils::mapRGB(m_image->format, label_bg));
    }
    {
        SDL_Rect rect = SDL_utils::makeRect(DIALOG_BORDER * screen.ppu_x,
            (DIALOG_BORDER + ((m_nbTitle ? 1 : 0) + m_nbLabels) * LINE_HEIGHT)
                * screen.ppu_y,
            m_image->w - 2 * DIALOG_BORDER * screen.ppu_x,
            (l_height - 2 * DIALOG_BORDER
                - ((m_nbTitle ? 1 : 0) + m_nbLabels) * LINE_HEIGHT)
                * screen.ppu_y);
        SDL_FillRect(m_image, &rect, SDL_MapRGB(m_image->format, COLOR_BG_1));
    }
    // Create cursor image
    m_cursor1 = SDL_utils::createImage(l_cursorWidth * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(screen.surface->format, COLOR_CURSOR_1));
    m_cursor2 = SDL_utils::createImage(l_cursorWidth * screen.ppu_x, LINE_HEIGHT * screen.ppu_y, SDL_MapRGB(screen.surface->format, COLOR_CURSOR_2));

    // Adjust dialog coordinates
    m_x = m_x_fn ? m_x_fn() : (screen.w - m_image->w / screen.ppu_x) / 2;
    if (!m_y_fn) {
        m_y = (screen.h - l_height) / 2;
    }
    else
    {
        m_y = m_y_fn();

        // Ensure the dialog fits vertically regardless of the requested
        // coordinates.
        m_y = m_y - (l_height >> 1) + (LINE_HEIGHT >> 1);
        if (m_y < Y_LIST) m_y = Y_LIST;
        if (m_y + l_height > FOOTER_Y + 1) m_y = FOOTER_Y + 1 - l_height;
    }
    // Cursor coordinates
    m_cursorX = m_x + DIALOG_BORDER;
    m_cursorY = m_y + DIALOG_BORDER + (m_nbTitle + m_nbLabels) * LINE_HEIGHT;
}

void CDialog::onResize()
{
    freeResources();
    init();
}

void CDialog::render(const bool p_focus) const
{
    INHIBIT(std::cout << "CDialog::render  fullscreen: " << isFullScreen() << "  focus: " << p_focus << std::endl;)
    // Draw background
    SDL_utils::applySurface(m_x, m_y, m_image, screen.surface);
    // Draw cursor
    SDL_utils::applySurface(m_cursorX, m_cursorY + m_highlightedLine * LINE_HEIGHT, p_focus ? m_cursor1 : m_cursor2, screen.surface);
    // Draw lines text
    Sint16 l_y = m_y + 4;
    if (m_nbTitle) {
        SDL_utils::applySurface(m_cursorX + DIALOG_MARGIN, l_y - 1, m_titleImg, screen.surface, &m_clip);
        l_y += LINE_HEIGHT;
    }
    for (int i = 0; i < m_linesImg.size(); ++i, l_y += LINE_HEIGHT) {
        SDL_Surface *surface;
        if (i == m_nbLabels + m_highlightedLine) {
            surface = p_focus ? m_linesImgCursor1[i - m_nbLabels] : m_linesImgCursor2[i - m_nbLabels];
        } else {
            surface = m_linesImg[i];
        }
        SDL_utils::applySurface(m_cursorX + DIALOG_MARGIN, l_y, surface, screen.surface, &m_clip);
    }
}

const bool CDialog::keyPress(const SDL_Event &p_event)
{
    CWindow::keyPress(p_event);
    bool l_ret(false);
    switch (p_event.key.keysym.sym)
    {
        case MYKEY_PARENT:
        case MYKEY_SYSTEM:
            m_retVal = -1;
            l_ret = true;
            break;
        case MYKEY_UP:
            l_ret = moveCursorUp(true);
            break;
        case MYKEY_DOWN:
            l_ret = moveCursorDown(true);
            break;
        case MYKEY_PAGEUP:
            if (m_highlightedLine)
            {
                m_highlightedLine = 0;
                l_ret = true;
            }
            break;
        case MYKEY_PAGEDOWN:
            if (m_highlightedLine + 1 < m_nbOptions)
            {
                m_highlightedLine = m_nbOptions - 1;
                l_ret = true;
            }
            break;
        case MYKEY_OPEN:
            m_retVal = m_highlightedLine + 1;
            l_ret = true;
            break;
        default:
            break;
    }
    return l_ret;
}

int CDialog::getLineAt(int x, int y) const {
    const int border_x = static_cast<int>(DIALOG_BORDER * screen.ppu_x);
    const int border_y = static_cast<int>(DIALOG_BORDER * screen.ppu_y);
    const int line_height = static_cast<int>(LINE_HEIGHT * screen.ppu_y);
    const int x0 = static_cast<int>(m_x * screen.ppu_x) + + border_x;
    const int x1 = x0 + width_ - 2 * border_x;
    const int y0 = static_cast<int>(m_y * screen.ppu_y) + border_y + (m_nbTitle + m_nbLabels) * line_height;
    const int y1 = y0 + m_nbOptions * line_height;
    if (x < x0 || x > x1 || y < y0 || y > y1) return -1;
    return (y - y0) / line_height;
}

bool CDialog::mouseDown(int button, int x, int y) {
    if (x < m_x * screen.ppu_x || x > m_x * screen.ppu_x + width_
        || y < m_y * screen.ppu_y || y > m_y * screen.ppu_y + height_)
    {
        m_retVal = -1;
        return true;
    }
    const int line = getLineAt(x, y);
    if (line == -1) return false;
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            m_highlightedLine = line;
            m_retVal = m_highlightedLine + 1;
            return true;
        case SDL_BUTTON_MIDDLE:
        case SDL_BUTTON_RIGHT:
            m_highlightedLine = line;
            return true;
        case SDL_BUTTON_X2: m_retVal = -1; return true;
#ifndef USE_SDL2
        case SDL_BUTTON_WHEELUP:
            return moveCursorUp(/*p_loop=*/false);
        case SDL_BUTTON_WHEELDOWN:
            return moveCursorDown(/*p_loop=*/false);
#endif
    }
    return false;
}

const bool CDialog::moveCursorUp(const bool p_loop)
{
    bool l_ret(false);
    if (m_highlightedLine)
    {
        --m_highlightedLine;
        l_ret = true;
    }
    else if (p_loop && m_highlightedLine + 1 < m_nbOptions)
    {
        m_highlightedLine = m_nbOptions - 1;
        l_ret = true;
    }
    return l_ret;
}

const bool CDialog::moveCursorDown(const bool p_loop)
{
    bool l_ret(false);
    if (m_highlightedLine + 1 < m_nbOptions)
    {
        ++m_highlightedLine;
        l_ret = true;
    }
    else if (p_loop && m_highlightedLine)
    {
        m_highlightedLine = 0;
        l_ret = true;
    }
    return l_ret;
}

const bool CDialog::keyHold(void)
{
    bool l_ret(false);
    switch(m_lastPressed)
    {
        case MYKEY_UP:
            if (tick(MYKEY_UP))
                l_ret = moveCursorUp(false);
            break;
        case MYKEY_DOWN:
            if (tick(MYKEY_DOWN))
                l_ret = moveCursorDown(false);
            break;
        default:
            break;
    }
    return l_ret;
}

const Sint16 &CDialog::getX(void) const
{
    return m_x;
}

const Sint16 &CDialog::getY(void) const
{
    return m_y;
}

const SDL_Surface * const CDialog::getImage(void) const
{
    return m_image;
}

const unsigned int &CDialog::getHighlightedIndex(void) const
{
    return m_highlightedLine;
}
