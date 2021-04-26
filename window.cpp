#include "window.h"

#include <cstdint>
#include <iostream>

#include "axis_direction.h"
#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"

#define KEYHOLD_TIMER_FIRST   12
#define KEYHOLD_TIMER         3

CWindow::CWindow(void)
    : m_keyHoldCountdown(0)
    , m_controllerButtonCountdown(0)
    , m_lastPressed(SDLK_0)
    , m_retVal(0)
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

int CWindow::execute()
{
#ifdef USE_SDL2
    const bool text_input_was_active = SDL_IsTextInputActive();
    SDL_StopTextInput();
    if (handlesTextInput()) SDL_StartTextInput();
#endif
    m_retVal = 0;
    SDL_Event event;
    bool l_loop(true);
    bool l_render(true);
    static AxisDirectionRepeater axisDirectionRepeater;
    static AxisDirection axisDirection;
    // Main loop
    while (l_loop)
    {
        // Handle key press
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_MOUSEMOTION:
                    SDL_utils::setMouseCursorEnabled(true);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    SDL_utils::setMouseCursorEnabled(true);
                    switch (event.button.button) {
#ifndef USE_SDL2
                        case SDL_BUTTON_WHEELUP:
                            l_render = mouseWheel(0, 1) || l_render;
                            break;
                        case SDL_BUTTON_WHEELDOWN:
                            l_render = mouseWheel(0, -1) || l_render;
                            break;
#endif
                        default:
                            l_render = this->mouseDown(event.button.button,
                                           event.button.x, event.button.y)
                                || l_render;
                    }
                    if (m_retVal) l_loop = false;
                    break;
                case SDL_KEYDOWN: {
                    SDL_utils::setMouseCursorEnabled(false);
                    if (handleZoomTrigger(event)) {
                        l_render = true;
                        break;
                    }
                    l_render = this->keyPress(event, event.key.keysym.sym,
                                   ControllerButton::NONE)
                        || l_render;
                    if (m_retVal) l_loop = false;
                    break;
                }
                case SDL_QUIT: return m_retVal;
#ifdef USE_SDL2
                case SDL_CONTROLLERDEVICEADDED:
                    SDL_GameControllerOpen(event.cdevice.which);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                case SDL_CONTROLLERBUTTONDOWN: {
                    bool thumbStickEvent = false;
                    if (event.type == SDL_CONTROLLERAXISMOTION) {
                        constexpr int AxisDeadzoneX = 16000;
                        constexpr int AxisDeadzoneY = 4000;
                        switch (event.caxis.axis) {
                            case SDL_CONTROLLER_AXIS_LEFTX:
                            case SDL_CONTROLLER_AXIS_RIGHTX: {
                                thumbStickEvent = true;
                                if (event.caxis.value < -AxisDeadzoneX
                                    || event.caxis.value > AxisDeadzoneX) {
                                    axisDirection.x = event.caxis.value > 0
                                        ? AxisDirectionX::RIGHT
                                        : AxisDirectionX::LEFT;
                                } else {
                                    axisDirection.x = AxisDirectionX::NONE;
                                }
                            } break;
                            case SDL_CONTROLLER_AXIS_LEFTY:
                            case SDL_CONTROLLER_AXIS_RIGHTY: {
                                thumbStickEvent = true;
                                if (event.caxis.value < -AxisDeadzoneY
                                    || event.caxis.value > AxisDeadzoneY) {
                                    axisDirection.y = event.caxis.value > 0
                                        ? AxisDirectionY::DOWN
                                        : AxisDirectionY::UP;
                                } else {
                                    axisDirection.y = AxisDirectionY::NONE;
                                }
                            } break;
                            default: break;
                        }
                    }
                    if (!thumbStickEvent) {
                        const ControllerButton button
                            = ControllerButtonFromSdlEvent(event);
                        SDL_utils::setMouseCursorEnabled(false);
                        l_render = this->keyPress(event, SDLK_UNKNOWN, button)
                            || l_render;
                        if (m_retVal) l_loop = false;
                    }
                    break;
                }
                case SDL_TEXTINPUT:
                case SDL_TEXTEDITING:
                    l_render = textInput(event) || l_render;
                    break;
                case SDL_MOUSEWHEEL:
                    SDL_utils::setMouseCursorEnabled(true);
                    l_render
                        = mouseWheel(event.wheel.x, event.wheel.y) || l_render;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_EXPOSED:
                            l_render = true;
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            l_render = true;
                            ResetFrameDeadline();
                            screen.onResize(
                                event.window.data1, event.window.data2);
                            triggerOnResize();
                            break;
                    }
                    break;
#else
                case SDL_VIDEORESIZE:
                    l_render = true;
                    ResetFrameDeadline();
                    screen.onResize(event.resize.w, event.resize.h);
                    triggerOnResize();
                    break;
#endif
            }
        }
        // Handle key hold
        if (!l_loop) break;

#if SDL_VERSION_ATLEAST(2, 0, 0)
        const int num_joysticks = SDL_NumJoysticks();
        for (int i = 0; i < num_joysticks; ++i) {
            if (!SDL_IsGameController(i)) continue;
            SDL_GameController *controller = SDL_GameControllerFromInstanceID(
                SDL_JoystickGetDeviceInstanceID(i));
            if (controller == nullptr) continue;
            l_render = this->gamepadHold(controller) || l_render;
        }

        // Thumb stick movement.
        AxisDirection dir = axisDirectionRepeater.Get(axisDirection);
        if (dir.x != AxisDirectionX::NONE) {
            l_render
                = this->keyPress(event, SDLK_UNKNOWN,
                      dir.x == AxisDirectionX::LEFT ? ControllerButton::LEFT
                                                    : ControllerButton::RIGHT)
                || l_render;
        }
        if (dir.y != AxisDirectionY::NONE) {
            l_render = this->keyPress(event, SDLK_UNKNOWN,
                           dir.y == AxisDirectionY::UP ? ControllerButton::UP
                                                       : ControllerButton::DOWN)
                || l_render;
        }
#endif

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

#ifdef USE_SDL2
    SDL_StopTextInput();
    if (text_input_was_active) SDL_StartTextInput();
#endif

    // -1 is used to signal cancellation but we must return 0 in that case.
    if (m_retVal == -1) m_retVal = 0;
    return m_retVal;
}

bool CWindow::handleZoomTrigger(const SDL_Event &event)
{
    if (event.type != SDL_KEYDOWN) return false;
    const auto sym = event.key.keysym.sym;
    // Zoom on CTRL +/-
    if ((SDL_GetModState() & KMOD_CTRL) == 0) return false;
    float factor;
    switch (sym) {
        case SDLK_PLUS:
        case SDLK_KP_PLUS: factor = 1.1f; break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS: factor = 1 / 1.1f; break;
        default: return false;
    }
    screen.zoom(factor);
    triggerOnResize();
    return true;
}

void CWindow::triggerOnResize() {
    CResourceManager::instance().onResize();
    for (auto *window : Globals::g_windows) window->onResize();
}

bool CWindow::keyPress(
    const SDL_Event &event, SDLC_Keycode key, ControllerButton button)
{
    // Reset timer if running
    if (m_keyHoldCountdown) m_keyHoldCountdown = 0;
    if (key != SDLK_UNKNOWN) m_lastPressed = key;
    if (button != ControllerButton::NONE) {
        m_controllerButtonCountdown = 0;
        m_lastPressedButton = button;
    }
    return false;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CWindow::keyHold() { return false; }

#if SDL_VERSION_ATLEAST(2, 0, 0)
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CWindow::gamepadHold([[maybe_unused]] SDL_GameController *controller)
{
    return false;
}
#endif

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool CWindow::textInput([[maybe_unused]] const SDL_Event &event)
{
    return false;
}

void CWindow::onResize() { }

bool CWindow::tick(SDLC_Keycode keycode)
{
    if (m_lastPressed != keycode) return false;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    const bool held
        = SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(keycode)];
#else
    const bool held = SDL_GetKeyState(NULL)[keycode];
#endif
    if (held) {
        if (m_keyHoldCountdown != 0) {
            --m_keyHoldCountdown;
            if (m_keyHoldCountdown == 0) {
                // Timer continues
                m_keyHoldCountdown = KEYHOLD_TIMER;
                // Trigger!
                return true;
            }
        } else {
            // Start timer
            m_keyHoldCountdown = KEYHOLD_TIMER_FIRST;
        }
    } else {
        // Stop timer if running
        if (m_keyHoldCountdown != 0) m_keyHoldCountdown = 0;
    }
    return false;
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
bool CWindow::tick(SDL_GameController *controller, ControllerButton button)
{
    if (controller == nullptr || m_lastPressedButton != button) return false;
    if (IsControllerButtonDown(controller, button)) {
        if (m_controllerButtonCountdown != 0) {
            --m_controllerButtonCountdown;
            if (m_controllerButtonCountdown == 0) {
                // Timer continues
                m_controllerButtonCountdown = KEYHOLD_TIMER;
                // Trigger!
                return true;
            }
        } else {
            // Start timer
            m_controllerButtonCountdown = KEYHOLD_TIMER_FIRST;
        }
    } else {
        // Stop timer if running
        if (m_controllerButtonCountdown != 0) m_controllerButtonCountdown = 0;
    }
    return false;
}
#endif

bool CWindow::mouseDown(int button, int x, int y) { return false; }
bool CWindow::mouseWheel(int dx, int dy) { return false; }

bool CWindow::isFullScreen(void) const
{
    // Default behavior
    return false;
}

bool CWindow::handlesTextInput() const
{
    // Default behavior
    return false;
}
