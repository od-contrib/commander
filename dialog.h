#ifndef _DIALOG_H_
#define _DIALOG_H_

#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>
#include "sdl_ttf_multifont.h"
#include "window.h"

class CDialog : public CWindow
{
    public:

    // Constructor
    // Coordinates = 0 => centered
    CDialog(const std::string &p_title, const Sint16 p_x = 0, const Sint16 p_y = 0);

    // Destructor
    virtual ~CDialog(void);

    // Add a label
    void addLabel(const std::string &p_label);

    // Add a menu option
    void addOption(const std::string &p_option);

    // Sets the color of the borders and title background.
    void setBorderColor(SDL_Color color) { m_borderColor = color; }

    // Init. Call after all options are added.
    void init(void);

    // Accessors
    const Sint16 &getX(void) const;
    const Sint16 &getY(void) const;
    const SDL_Surface * const getImage(void) const;
    const unsigned int &getHighlightedIndex(void) const;

    private:

    // Forbidden
    CDialog(void);
    CDialog(const CDialog &p_source);
    const CDialog &operator =(const CDialog &p_source);

    // Key press management
    virtual const bool keyPress(const SDL_Event &p_event);

    // Key hold management
    virtual const bool keyHold(void);

    // Draw
    virtual void render(const bool p_focus) const;

    // Move cursor
    const bool moveCursorUp(const bool p_loop);
    const bool moveCursorDown(const bool p_loop);

    SDL_Color m_borderColor;

    // Number of titles (0 or 1), labels, and options
    bool m_nbTitle;
    unsigned char m_nbLabels;
    unsigned char m_nbOptions;

    // List of lines
    std::vector<std::string> m_lines;
    SDL_Surface *m_titleImg;
    std::vector<SDL_Surface *> m_linesImg;
    std::vector<SDL_Surface *> m_linesImgCursor1;
    std::vector<SDL_Surface *> m_linesImgCursor2;

    // The highlighted item
    unsigned int m_highlightedLine;

    // The image representing the dialog
    SDL_Surface *m_image;

    // The cursor
    SDL_Surface *m_cursor1;
    SDL_Surface *m_cursor2;

    // Coordinates
    Sint16 m_x;
    Sint16 m_y;
    Sint16 m_cursorX;
    Sint16 m_cursorY;

    // Line clip
    mutable SDL_Rect m_clip;

    // Pointers to resources
    const Fonts &m_fonts;
};

#endif
