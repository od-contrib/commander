#include "commander.h"

#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>

#include "def.h"
#include "dialog.h"
#include "error_dialog.h"
#include "file_info.h"
#include "fileutils.h"
#include "keyboard.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"
#include "viewer.h"

#define SPLITTER_LINE_W 1
#define X_LEFT 1
#define X_RIGHT (screen.w / 2 + SPLITTER_LINE_W + 1)

namespace {

SDL_Surface *DrawBackground() {
    SDL_Surface *bg = SDL_utils::createSurface(screen.w * screen.ppu_x, screen.h * screen.ppu_y);

    // Stripes
    const int stripes_h = screen.h - HEADER_H - FOOTER_H;
    SDL_Rect rect = SDL_utils::Rect(0, 0, screen.w * screen.ppu_x, screen.h * screen.ppu_y);
    const Uint32 bg_colors[2] = {SDL_MapRGB(bg->format, COLOR_BG_1), SDL_MapRGB(bg->format, COLOR_BG_2)};
    const std::size_t num_lines = (stripes_h - 1) / LINE_HEIGHT + 1;
    for (std::size_t i = 0; i < num_lines; ++i) {
        rect.y = (Y_LIST + i * LINE_HEIGHT) * screen.ppu_y;
        SDL_FillRect(bg, &rect, bg_colors[i % 2]);
    }

    // Top and bottom bars
    const auto bar_color = SDL_MapRGB(bg->format, COLOR_TITLE_BG);
    rect = SDL_utils::Rect(0, 0, static_cast<decltype(SDL_Rect().w)>(bg->w), Y_LIST * screen.ppu_y);
    SDL_FillRect(bg, &rect, bar_color);
    rect.y = bg->h - FOOTER_H * screen.ppu_y;
    SDL_FillRect(bg, &rect, bar_color);

    // Line in the middle
    rect = SDL_utils::Rect(screen.w / 2 * screen.ppu_x, 0, SPLITTER_LINE_W * screen.ppu_x, Y_LIST * screen.ppu_y);
    SDL_FillRect(bg, &rect, bg_colors[0]);
    rect.y = rect.h;
    rect.h = stripes_h * screen.ppu_y;
    SDL_FillRect(bg, &rect, bar_color);
    rect.y += rect.h;
    rect.h = FOOTER_H * screen.ppu_y;
    SDL_FillRect(bg, &rect, bg_colors[0]);

    return bg;
}

} // namespace

CCommander::CCommander(const std::string &p_pathL, const std::string &p_pathR):
    CWindow::CWindow(),
    m_panelLeft(p_pathL, X_LEFT),
    m_panelRight(p_pathR, X_RIGHT),
    m_panelSource(NULL),
    m_panelTarget(NULL),
    m_background(DrawBackground())
{
    m_panelSource = &m_panelLeft;
    m_panelTarget = &m_panelRight;
}

CCommander::~CCommander(void)
{
    SDL_FreeSurface(m_background);
}

void CCommander::render(const bool p_focus) const
{
    INHIBIT(std::cout << "CCommander::render  fullscreen: " << isFullScreen() << "  focus: " << p_focus << std::endl;)
    // Draw background image
    SDL_utils::applySurface(0, 0, m_background, screen.surface);
    // Draw panels
    m_panelLeft.render(p_focus && (m_panelSource == &m_panelLeft));
    m_panelRight.render(p_focus && (m_panelSource == &m_panelRight));
}

void CCommander::onResize()
{
    SDL_FreeSurface(m_background);
    m_background = DrawBackground();
    m_panelRight.setX(X_RIGHT);
}

const bool CCommander::keyPress(const SDL_Event &p_event)
{
    CWindow::keyPress(p_event);
    bool l_ret(false);
    switch (p_event.key.keysym.sym)
    {
        case MYKEY_SYSTEM:
            if (openSystemMenu())
                m_panelSource->refresh();
            l_ret = true;
            break;
        case MYKEY_UP:
            l_ret = m_panelSource->moveCursorUp(1);
            break;
        case MYKEY_DOWN:
            l_ret = m_panelSource->moveCursorDown(1);
            break;
        case MYKEY_PAGEUP:
            l_ret = m_panelSource->moveCursorUp(NB_VISIBLE_LINES - 1);
            break;
        case MYKEY_PAGEDOWN:
            l_ret = m_panelSource->moveCursorDown(NB_VISIBLE_LINES - 1);
            break;
        case MYKEY_LEFT:
            if (m_panelSource == &m_panelRight)
            {
                m_panelSource = &m_panelLeft;
                m_panelTarget = &m_panelRight;
                l_ret = true;
            }
            break;
        case MYKEY_RIGHT:
            if (m_panelSource == &m_panelLeft)
            {
                m_panelSource = &m_panelRight;
                m_panelTarget = &m_panelLeft;
                l_ret = true;
            }
            break;
        case MYKEY_OPEN:
            l_ret = itemMenu();
            break;
        case MYKEY_PARENT:
            l_ret = m_panelSource->goToParentDir();
            break;
        case MYKEY_OPERATION:
            // If there's no file in the select list, add current file
            if (m_panelSource->getSelectList().empty() && m_panelSource->getHighlightedItem() != "..")
                m_panelSource->addToSelectList(false);
            l_ret = operationMenu();
            break;
        case MYKEY_SELECT:
            l_ret = m_panelSource->addToSelectList(true);
            break;
        case MYKEY_TRANSFER:
            if (m_panelSource->isDirectoryHighlighted() && m_panelSource->getHighlightedItem() != "..")
                l_ret = m_panelTarget->open(m_panelSource->getHighlightedItemFull());
            else
                l_ret = m_panelTarget->open(m_panelSource->getCurrentPath());
            break;
        default:
            break;
    }
    return l_ret;
}

const bool CCommander::keyHold(void)
{
    bool l_ret(false);
    switch(m_lastPressed)
    {
        case MYKEY_UP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_UP]))
                l_ret = m_panelSource->moveCursorUp(1);
            break;
        case MYKEY_DOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_DOWN]))
                l_ret = m_panelSource->moveCursorDown(1);
            break;
        case MYKEY_PAGEUP:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_PAGEUP]))
                l_ret = m_panelSource->moveCursorUp(NB_VISIBLE_LINES - 1);
            break;
        case MYKEY_PAGEDOWN:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_PAGEDOWN]))
                l_ret = m_panelSource->moveCursorDown(NB_VISIBLE_LINES - 1);
            break;
        case MYKEY_SELECT:
            if (tick(SDL_GetKeyState(NULL)[MYKEY_SELECT]))
                l_ret = m_panelSource->addToSelectList(true);
            break;
        default:
            break;
    }
    return l_ret;
}

bool CCommander::mouseDown(int button, int x, int y)
{
    if (x < X_LEFT * screen.ppu_x) return false;
    bool changed = false;

    CPanel *target;
    if (x >= X_RIGHT * screen.ppu_x)
    {
        target = &m_panelRight;
        x -= X_RIGHT * screen.ppu_x;
    }
    else
    {
        target = &m_panelLeft;
        x -= X_LEFT * screen.ppu_x;
    }
    if (m_panelSource != target)
    {
        m_panelTarget = m_panelSource;
        m_panelSource = target;
        changed = true;
    }

    const int line = target->getLineAt(x, y);
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            if (line == -1) { openSystemMenu(); }
            else
            {
                target->moveCursorToVisibleLineIndex(line);
                itemMenu();
            }
            return true;
        case SDL_BUTTON_MIDDLE:
            if (line != -1)
            {
                target->moveCursorToVisibleLineIndex(line);
                target->addToSelectList(/*p_step=*/false);
                return true;
            }
            return changed;
        case SDL_BUTTON_RIGHT:
            if (m_panelSource->getSelectList().empty()) {
                if (line == -1) {
                    openSystemMenu();
                    return true;
                }
                target->moveCursorToVisibleLineIndex(line);
                target->addToSelectList(/*p_step=*/false);
                changed = true;
            }
            return operationMenu() || changed;
        case SDL_BUTTON_X2: return target->goToParentDir() || changed;
        case SDL_BUTTON_WHEELUP:
            return target->moveCursorUp(1) || changed;
        case SDL_BUTTON_WHEELDOWN:
            return target->moveCursorDown(1) || changed;
    }
    return changed;
}

bool CCommander::itemMenu() const
{
    if (m_panelSource->isDirectoryHighlighted())
        return m_panelSource->open();
    // It's a file => open execute menu
    openExecuteMenu();
    return true;
}

bool CCommander::operationMenu() const
{
    if (m_panelSource->getSelectList().empty()) return false;
    if (openCopyMenu())
    {
        // Refresh file lists
        m_panelSource->refresh();
        m_panelTarget->refresh();
    }
    else
    {
        if (m_panelSource->getSelectList().size() == 1
            && (*m_panelSource->getSelectList().begin())
                == m_panelSource->getHighlightedIndex())
            m_panelSource->selectNone();
    }
    return true;
}

const bool CCommander::openCopyMenu(void) const
{
    std::vector<std::function<bool()>> handlers;
    int l_dialogRetVal(0);
    // List of selected files
    std::vector<std::string> l_list;
    m_panelSource->getSelectList(l_list);
    {
        bool l_loop(false);
        std::ostringstream l_stream;
        l_stream << l_list.size() << " selected:";
        // File operation dialog
        CDialog l_dialog { l_stream.str(), {}, [this]() {
                              return Y_LIST
                                  + m_panelSource->getHighlightedIndexRelative()
                                  * LINE_HEIGHT;
                          } };

        l_dialog.addOption(m_panelSource == &m_panelLeft ? "Copy >" : "< Copy");
        handlers.push_back([&]() {
            File_utils::copyFile(l_list, m_panelTarget->getCurrentPath());
            return true;
        });

        l_dialog.addOption(m_panelSource == &m_panelLeft ? "Move >" : "< Move");
        handlers.push_back([&]() {
            File_utils::moveFile(l_list, m_panelTarget->getCurrentPath());
            return true;
        });

        
        l_dialog.addOption(m_panelSource == &m_panelLeft ? "Symlink >" : "< Symlink");
        handlers.push_back([&]() {
            File_utils::symlinkFile(l_list, m_panelTarget->getCurrentPath());
            return true;
        });

        if (l_list.size() == 1) {
            // The rename option appears only if one item is selected
            l_dialog.addOption("Rename");
            handlers.push_back([&]() {
                CKeyboard l_keyboard(m_panelSource->getHighlightedItem());
                if (l_keyboard.execute() == 1 && !l_keyboard.getInputText().empty() && l_keyboard.getInputText() != m_panelSource->getHighlightedItem())
                {
                    File_utils::renameFile(m_panelSource->getHighlightedItemFull(), m_panelSource->getCurrentPath() + (m_panelSource->getCurrentPath() == "/" ? "" : "/") + l_keyboard.getInputText());
                    return true;
                }
                return false;
            });
        }

        l_dialog.addOption("Delete");
        handlers.push_back([&]() {
            File_utils::removeFile(l_list);
            return true;
        });
        const int delete_option = handlers.size(); 

        l_dialog.addOption("Disk used");
        handlers.push_back([&]() {
            File_utils::diskUsed(l_list);
            return false;
        });

        l_dialog.init();
        do
        {
            l_loop = false;
            l_dialogRetVal = l_dialog.execute();
            if (l_dialogRetVal == delete_option)
            {
                CDialog l_dialog2 { "",
                    [&]() {
                        return l_dialog.getX()
                            + l_dialog.getImage()->w / screen.ppu_x
                            - DIALOG_BORDER;
                    },
                    [&]() {
                        return l_dialog.getY() / screen.ppu_y + DIALOG_BORDER
                            + (l_dialog.getHighlightedIndex() + 1)
                            * LINE_HEIGHT;
                    } };
                l_dialog2.addOption("Yes");
                l_dialog2.addOption("No");
                l_dialog2.init();
                if (l_dialog2.execute() != 1)
                    l_loop = true;
            }
        }
        while (l_loop);
    }
    // Perform operation
    if (l_dialogRetVal > 0 && l_dialogRetVal <= handlers.size()) {
        return handlers[l_dialogRetVal - 1]();
    }
    return false;
}

const bool CCommander::openSystemMenu(void)
{
    bool l_ret(false);
    int l_dialogRetVal(0);
    // Selection dialog
    {
        CDialog l_dialog { "System:", {}, [this]() {
                              return Y_LIST
                                  + m_panelSource->getHighlightedIndexRelative()
                                  * LINE_HEIGHT;
                          } };
        l_dialog.addOption("Select all");
        l_dialog.addOption("Select none");
        l_dialog.addOption("New directory");
        l_dialog.addOption("Disk info");
        l_dialog.addOption("Quit");
        l_dialog.init();
        l_dialogRetVal = l_dialog.execute();
    }
    switch (l_dialogRetVal)
    {
        case 1:
            // Select all
            m_panelSource->selectAll();
            break;
        case 2:
            // Select none
            m_panelSource->selectNone();
            break;
        case 3:
            // New dir
            {
                CKeyboard l_keyboard("");
                if (l_keyboard.execute() == 1 && !l_keyboard.getInputText().empty())
                {
                    File_utils::makeDirectory(m_panelSource->getCurrentPath() + (m_panelSource->getCurrentPath() == "/" ? "" : "/") + l_keyboard.getInputText());
                    m_panelSource->refresh();
                    l_ret = true;
                }
            }
            break;
        case 4:
            // Disk info
            File_utils::diskInfo();
            break;
        case 5:
            // Quit
            m_retVal = -1;
            break;
        default:
            break;
    }
    return l_ret;
}

namespace {

enum class OpenFileResult
{
    CANCEL,
    VIEW,
    EXECUTE
};

OpenFileResult OpenFileDialog(const std::string &path,
    std::function<Sint16()> x_fn = {}, std::function<Sint16()> y_fn = {})
{
    const auto info = FileInfo::Get(path);

    if (File_utils::getLowercaseFileExtension(path) == "opk")
        return OpenFileResult::EXECUTE;

    if (!info.executable()) return OpenFileResult::VIEW;

    CDialog dlg { File_utils::getFileName(path) + ":" };
    std::vector<OpenFileResult> options { OpenFileResult::CANCEL };
    const auto add_option = [&](std::string text, OpenFileResult value) {
        dlg.addOption(text);
        options.push_back(value);
    };
    add_option("View", OpenFileResult::VIEW);
    add_option("Execute", OpenFileResult::EXECUTE);
    dlg.init();
    return options[dlg.execute()];
}

void ViewFile(const std::string &path)
{
    // Check size
    const auto file_size = File_utils::getFileSize(path);
    if (file_size > VIEWER_SIZE_MAX)
        ErrorDialog(path, "Error:", "File too large (>16 MiB)");
    else
        CViewer(path).execute();
}

} // namespace

void CCommander::openExecuteMenu(void) const
{
    switch (
        OpenFileDialog(m_panelSource->getHighlightedItemFull(), {}, [this]() {
            return Y_LIST
                + m_panelSource->getHighlightedIndexRelative() * LINE_HEIGHT;
        }))
    {
        case OpenFileResult::VIEW:
            ViewFile(m_panelSource->getHighlightedItemFull());
            break;
        case OpenFileResult::EXECUTE:
            File_utils::executeFile(m_panelSource->getHighlightedItemFull());
            break;
        case OpenFileResult::CANCEL:
            break;
    }
}

bool CCommander::isFullScreen(void) const
{
    return true;
}
