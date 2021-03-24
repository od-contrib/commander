#include "window.h"

#include <cstdint>
#include <iostream>

#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

#define KEYHOLD_TIMER_FIRST   12
#define KEYHOLD_TIMER         3

CWindow::CWindow(void):
    m_timer(0),
    m_lastPressed(SDLK_0),
    m_retVal(0)
{
    // Add window to the lists for render
    Globals::g_windows.push_back(this);
}

CWindow::~CWindow(void)
{
    // Remove last window
    Globals::g_windows.pop_back();
}

namespace
{

std::uint32_t frameDeadline = 0;

// Limit FPS to avoid high CPU load, use when v-sync isn't available
void LimitFrameRate()
{
    const int refreshDelay = 1000000 / screen.refreshRate;
    std::uint32_t tc = SDL_GetTicks() * 1000;
    std::uint32_t v = 0;
    if (frameDeadline > tc)
    {
        v = tc % refreshDelay;
        SDL_Delay(v / 1000 + 1); // ceil
    }
    frameDeadline = tc + v + refreshDelay;
}

void ResetFrameDeadline() {
    frameDeadline = 0;
}

} // namespace

const int CWindow::execute(void)
{
    m_retVal = 0;
    SDL_Event event;
    bool l_loop(true);
    bool l_render(true);
    // Main loop
    while (l_loop)
    {
        // Handle key press
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                    l_render = this->keyPress(event);
                    if (m_retVal) l_loop = false;
                    break;
                case SDL_QUIT: return m_retVal;
                case SDL_VIDEORESIZE:
                    l_render = true;
                    ResetFrameDeadline();
                    screen.onResize(event.resize.w, event.resize.h);
                    CResourceManager::instance().onResize();
                    for (auto *window : Globals::g_windows) window->onResize();
                    break;
            }
        }
        // Handle key hold
        if (!l_loop) break;
        l_render = this->keyHold() || l_render;
        // Render if necessary
        if (l_render)
        {
            SDL_utils::renderAll();
            screen.flip();
            l_render = false;
        }
        LimitFrameRate();
    }

    // -1 is used to signal cancellation but we must return 0 in that case.
    if (m_retVal == -1) m_retVal = 0;
    return m_retVal;
}

const bool CWindow::keyPress(const SDL_Event &p_event)
{
    // Reset timer if running
    if (m_timer)
        m_timer = 0;
    m_lastPressed = p_event.key.keysym.sym;
    return false;
}

const bool CWindow::keyHold(void)
{
    // Default behavior
    return false;
}

void CWindow::onResize() { }

const bool CWindow::tick(const Uint8 p_held)
{
    bool l_ret(false);
    if (p_held)
    {
        if (m_timer)
        {
            --m_timer;
            if (!m_timer)
            {
                // Trigger!
                l_ret = true;
                // Timer continues
                m_timer = KEYHOLD_TIMER;
            }
        }
        else
        {
            // Start timer
            m_timer = KEYHOLD_TIMER_FIRST;
        }
    }
    else
    {
        // Stop timer if running
        if (m_timer)
            m_timer = 0;
    }
    return l_ret;
}

const int CWindow::getReturnValue(void) const
{
    return m_retVal;
}

bool CWindow::isFullScreen(void) const
{
    // Default behavior
    return false;
}
