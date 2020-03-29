#ifndef _VIEWER_H_
#define _VIEWER_H_

#include <cstddef>
#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>

#include "screen.h"
#include "window.h"

#define VIEWER_LINE_HEIGHT   13
#define VIEWER_Y_LIST        18
#define VIEWER_NB_LINES      ((screen.h - VIEWER_Y_LIST - 1) / VIEWER_LINE_HEIGHT + 1)
#define VIEWER_MARGIN        1
#define VIEWER_X_STEP        32
#define VIEWER_SIZE_MAX      16777216  // = 16 MB

class CViewer : public CWindow
{
    public:

    // Constructor
    CViewer(const std::string &p_fileName);

    // Destructor
    virtual ~CViewer(void);

    private:

    // Forbidden
    CViewer(void);
    CViewer(const CViewer &p_source);
    const CViewer &operator =(const CViewer &p_source);

    // Key press management
    virtual const bool keyPress(const SDL_Event &p_event);

    // Key hold management
    virtual const bool keyHold(void);

    // Draw
    virtual void render(const bool p_focus) const;

    // Is window full screen?
    virtual bool isFullScreen(void) const;

    // Scroll (text mode only)
    bool moveUp(const unsigned int p_step);
    bool moveDown(const unsigned int p_step);
    bool moveLeft(void);
    bool moveRight(void);

    // The viewed file name
    std::string m_fileName;

    // Coordinates
    mutable SDL_Rect m_clip;

    const std::vector<TTF_Font *> &m_fonts;

    // Background image
    SDL_Surface *m_background;

    enum {
        TEXT = 0,
        IMAGE = 1,
    } m_mode;

    // Text mode:
    std::size_t m_firstLine;

    // List of read lines
    std::vector<std::string> m_lines;

    // Image mode:
    SDL_Surface *m_image;
};

#endif
