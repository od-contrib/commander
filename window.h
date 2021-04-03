#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <SDL.h>

class CWindow
{
    public:

    // Destructor
    virtual ~CWindow(void);

    // Execute main loop of the window
    const int execute(void);

    // Return value
    const int getReturnValue(void) const;

    // Draw window
    virtual void render(const bool p_focus) const = 0;

    // Is window full screen?
    virtual bool isFullScreen(void) const;

    protected:

    // Constructor
    CWindow(void);

    // Window resized.
    virtual void onResize();

    // Mouse down event.
    // Return true if re-render is needed after handling this.
    virtual bool mouseDown(int button, int x, int y);

    // Key press management
    virtual const bool keyPress(const SDL_Event &p_event);

    // Key hold management
    virtual const bool keyHold(void);

    // Timer tick
#ifdef USE_SDL2
    bool tick(SDL_Keycode p_held);
#else
    bool tick(SDLKey p_held);
#endif

    // Timer for key hold
    unsigned int m_timer;

#ifdef USE_SDL2
    SDL_Keycode m_lastPressed;
#else
    SDLKey m_lastPressed;
#endif

    // Return value
    int m_retVal;

    private:

    // Forbidden
    CWindow(const CWindow &p_source);
    const CWindow &operator =(const CWindow &p_source);

};

#endif
