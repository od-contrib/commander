#include "controller_buttons.h"

#if SDL_VERSION_ATLEAST(2, 0, 0)
ControllerButton ControllerButtonFromSdlEvent(const SDL_Event &event)
{
    static bool trigger_left_is_down;
    static bool trigger_right_is_down;

    switch (event.type) {
        case SDL_CONTROLLERAXISMOTION:
            switch (event.caxis.axis) {
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                    if (event.caxis.value < 8192) { // 25% pressed
                        trigger_left_is_down = false;
                    }
                    if (event.caxis.value > 16384
                        && !trigger_left_is_down) { // 50% pressed
                        trigger_left_is_down = true;
                        return ControllerButton::TRIGGERLEFT;
                    }
                    return ControllerButton::NONE;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    if (event.caxis.value < 8192) { // 25% pressed
                        trigger_right_is_down = false;
                    }
                    if (event.caxis.value > 16384
                        && !trigger_right_is_down) { // 50% pressed
                        trigger_right_is_down = true;
                        return ControllerButton::TRIGGERRIGHT;
                    }
                    return ControllerButton::NONE;
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A: return ControllerButton::A;
                case SDL_CONTROLLER_BUTTON_B: return ControllerButton::B;
                case SDL_CONTROLLER_BUTTON_X: return ControllerButton::X;
                case SDL_CONTROLLER_BUTTON_Y: return ControllerButton::Y;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                    return ControllerButton::LEFTSTICK;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                    return ControllerButton::RIGHTSTICK;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    return ControllerButton::LEFTSHOULDER;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    return ControllerButton::RIGHTSHOULDER;
                case SDL_CONTROLLER_BUTTON_START:
                    return ControllerButton::START;
                case SDL_CONTROLLER_BUTTON_BACK:
                    return ControllerButton::SELECT;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: return ControllerButton::UP;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    return ControllerButton::DOWN;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    return ControllerButton::LEFT;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    return ControllerButton::RIGHT;
                default: break;
            }
        default: break;
    }
    return ControllerButton::NONE;
}
#endif
