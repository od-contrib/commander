#include <algorithm>
#include <iostream>
#include <fstream>

#include "viewer.h"
#include "resourceManager.h"
#include "def.h"
#include "sdlutils.h"

namespace {

inline int UTF8CodePointLen(const char* src) {
  return "\1\1\1\1\1\1\1\1\1\1\1\1\2\2\3\4"[static_cast<unsigned char>(*src)>> 4];
}

void ReplaceTabs(std::string *line) {
    constexpr std::size_t kTabWidth = 8;
    const std::size_t num_tabs = std::count(line->begin(), line->end(), '\t');
    if (num_tabs == 0) return;
    std::string result;
    result.reserve(line->size() + num_tabs * (kTabWidth - 1));
    std::size_t prev_tab_end = 0;
    std::size_t column = 0;
    for (std::size_t i = 0; i < line->size(); i += UTF8CodePointLen(line->data() + i)) {
        if ((*line)[i] == '\t') {
            result.append(*line, prev_tab_end, i - prev_tab_end);
            const std::size_t num_spaces = kTabWidth - (column % kTabWidth);
            result.append(num_spaces, ' ');
            prev_tab_end = i + 1;
            column += num_spaces;
        } else {
            ++column;
        }
    }
    result.append(*line, prev_tab_end, std::string::npos);
    *line = std::move(result);
}

} // namespace

CViewer::CViewer(const std::string &p_fileName):
    CWindow(),
    m_fileName(p_fileName),
    m_font(CResourceManager::instance().getFont()),
    m_background(nullptr),
    m_firstLine(0),
    m_image(nullptr)
{
    // Create background image
    m_background = SDL_utils::createImage(SCREEN_WIDTH, SCREEN_HEIGHT * PPU_Y, SDL_MapRGB(Globals::g_screen->format, COLOR_BG_1));
    {
        SDL_Rect l_rect = {0, 0, SCREEN_WIDTH, Y_LIST * PPU_Y};
        SDL_FillRect(m_background, &l_rect, SDL_MapRGB(m_background->format, COLOR_BORDER));
    }
    // Print title
    SDL_Surface *l_surfaceTmp = SDL_utils::renderText(m_font, m_fileName, Globals::g_colorTextTitle, {COLOR_TITLE_BG});
    if (l_surfaceTmp->w > m_background->w - 2 * VIEWER_MARGIN)
    {
        SDL_Rect l_rect;
        l_rect.x = l_surfaceTmp->w - (m_background->w - 2 * VIEWER_MARGIN);
        l_rect.y = 0;
        l_rect.w = m_background->w - 2 * VIEWER_MARGIN;
        l_rect.h = l_surfaceTmp->h;
        SDL_utils::applySurface(VIEWER_MARGIN, Y_HEADER, l_surfaceTmp, m_background, &l_rect);
    }
    else
    {
        SDL_utils::applySurface(VIEWER_MARGIN, Y_HEADER, l_surfaceTmp, m_background);
    }
    m_clip.h = l_surfaceTmp->h;
    SDL_FreeSurface(l_surfaceTmp);

    // Read file
    m_image = SDL_utils::loadImageToFit(m_fileName, SCREEN_WIDTH, SCREEN_HEIGHT - Y_LIST);
    if (m_image != nullptr)
    {
        m_mode = IMAGE;

        // Transparency grid background.
        constexpr int kTransparentBgRectSize = 10;
        const Uint32 colors[2] = {
            SDL_MapRGB(m_background->format, 240, 240, 240),
            SDL_MapRGB(m_background->format, 155, 155, 155),
        };
        int i = 0;
        for (int y = Y_LIST; y < SCREEN_HEIGHT; y += kTransparentBgRectSize, ++i) {
            for (int x = 0; x < SCREEN_WIDTH; x += kTransparentBgRectSize, ++i) {
                SDL_Rect rect = {x, y * PPU_Y, kTransparentBgRectSize, kTransparentBgRectSize * PPU_Y};
                SDL_FillRect(m_background, &rect, colors[i % 2]);
            }
        }
    }
    else
    {
        m_mode = TEXT;

        // Init clip rect
        m_clip.x = 0;
        m_clip.y = 0;
        m_clip.w = SCREEN_WIDTH - 2 * VIEWER_MARGIN;

        std::ifstream l_file(m_fileName.c_str());

        if (l_file.is_open())
        {
            while (!l_file.eof())
            {
                m_lines.emplace_back();
                std::getline(l_file, m_lines.back());
                ReplaceTabs(&m_lines.back());
            }
            l_file.close();
        }
        else
            std::cerr << "Error: unable to open file " << m_fileName << std::endl;
        INHIBIT(std::cout << "CViewer: " << m_lines.size() << " lines read" << std::endl;)
    }
}

CViewer::~CViewer(void)
{
    // Free surfaces
    if (m_image != NULL)
        SDL_FreeSurface(m_image);
    if (m_background != NULL)
        SDL_FreeSurface(m_background);
}

void CViewer::render(const bool p_focus) const
{
    INHIBIT(std::cout << "CViewer::render  fullscreen: " << isFullScreen() << "  focus: " << p_focus << std::endl;)
    // Draw background
    SDL_utils::applySurface(0, 0, m_background, Globals::g_screen);
    if (m_mode == IMAGE)
    {
        SDL_utils::applySurface((SCREEN_WIDTH - m_image->w) / 2, Y_LIST + (SCREEN_HEIGHT - Y_LIST - m_image->h / PPU_Y) / 2, m_image, Globals::g_screen);
    }
    else if (m_mode == TEXT)
    {
        // Draw lines
        std::size_t i = std::min(m_firstLine + VIEWER_NB_LINES, m_lines.size());
        while (i-- > m_firstLine)
        {
            const std::string &line = m_lines[i];
            if (line.empty())
                continue;
            SDL_Surface *l_surfaceTmp = SDL_utils::renderText(m_font, line, Globals::g_colorTextNormal, {COLOR_BG_1});
            SDL_utils::applySurface(VIEWER_MARGIN, VIEWER_Y_LIST + (i - m_firstLine) * VIEWER_LINE_HEIGHT, l_surfaceTmp, Globals::g_screen, &m_clip);
            SDL_FreeSurface(l_surfaceTmp);
        }
    }
}

const bool CViewer::keyPress(const SDL_Event &p_event)
{
    CWindow::keyPress(p_event);
    switch (p_event.key.keysym.sym)
    {
        case MYKEY_PARENT:
            m_retVal = -1;
            return true;
            break;
        case MYKEY_UP:
            if (m_mode == TEXT)
                return moveUp(1);
            break;
        case MYKEY_DOWN:
            if (m_mode == TEXT)
                return moveDown(1);
            break;
        case MYKEY_PAGEUP:
            if (m_mode == TEXT)
                return moveUp(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_PAGEDOWN:
            if (m_mode == TEXT)
                return moveDown(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_LEFT:
            if (m_mode == TEXT)
                return moveLeft();
            break;
        case MYKEY_RIGHT:
            if (m_mode == TEXT)
                return moveRight();
            break;
        default:
            break;
    }
    return false;
}

const bool CViewer::keyHold(void)
{
    switch(m_lastPressed)
    {
        case MYKEY_UP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_UP]))
                return moveUp(1);
            break;
        case MYKEY_DOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_DOWN]))
                return moveDown(1);
            break;
        case MYKEY_PAGEUP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_PAGEUP]))
                return moveUp(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_PAGEDOWN:
            if (m_mode == TEXT && tick(SDL_GetKeyState(NULL)[MYKEY_PAGEDOWN]))
                return moveDown(VIEWER_NB_LINES - 1);
            break;
        case MYKEY_LEFT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_LEFT]))
                return moveLeft();
            break;
        case MYKEY_RIGHT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_RIGHT]))
                return moveRight();
            break;
        default:
            break;
    }
    return false;
}

bool CViewer::isFullScreen(void) const
{
    return true;
}

bool CViewer::moveUp(const unsigned int p_step)
{
    bool l_ret(false);
    if (m_firstLine)
    {
        if (m_firstLine > p_step)
            m_firstLine -= p_step;
        else
            m_firstLine = 0;
        l_ret = true;
    }
    return l_ret;
}

bool CViewer::moveDown(const unsigned int p_step)
{
    bool l_ret(false);
    if (m_firstLine + VIEWER_NB_LINES + 1 < m_lines.size())
    {
        if (m_firstLine + VIEWER_NB_LINES + 1 + p_step > m_lines.size())
            m_firstLine = m_lines.size() - VIEWER_NB_LINES - 1;
        else
            m_firstLine += p_step;
        l_ret = true;
    }
    return l_ret;
}

bool CViewer::moveLeft(void)
{
    bool l_ret(false);
    if (m_clip.x > 0)
    {
        if (m_clip.x > VIEWER_X_STEP)
            m_clip.x -= VIEWER_X_STEP;
        else
            m_clip.x = 0;
        l_ret = true;
    }
    return l_ret;
}

bool CViewer::moveRight(void)
{
    m_clip.x += VIEWER_X_STEP;
    return true;
}
