#include "image_viewer.h"

#include "config.h"
#include "def.h"
#include "resourceManager.h"
#include "screen.h"
#include "sdlutils.h"
#include "text_viewer.h"

ImageViewer::ImageViewer(CPanel *panel)
    : panel_(panel), showTitle_(false)
{
    init();
    setPath(panel->getHighlightedItemFull());
}

void ImageViewer::init()
{
    // Create the fullscreen background surface
    background_ = SDLSurfaceUniquePtr{
        SDL_utils::createImage(screen.actual_w,
            screen.actual_h, SDL_MapRGB(screen.surface->format, COLOR_BG_1))
    };
    // Checkerboard background
    constexpr int kTransparentBgRectSize = 10;
    const Uint32 colors[2] = {
        SDL_MapRGB(background_->format, 240, 240, 240),
        SDL_MapRGB(background_->format, 155, 155, 155),
    };
    const int rect_w = static_cast<int>(kTransparentBgRectSize * screen.ppu_x);
    const int rect_h = static_cast<int>(kTransparentBgRectSize * screen.ppu_y);

    for (int j = 0, y = 0; y < screen.actual_h; y += rect_h, ++j) {
        for (int i = 0, x = 0; x < screen.actual_w; x += rect_w, ++i) {
            SDL_Rect rect = SDL_utils::makeRect(x, y, rect_w, rect_h);
            SDL_FillRect(background_.get(), &rect, colors[(i + j) % 2]);
        }
    }
}

void ImageViewer::setPath(std::string &&path)
{
    filename_ = std::move(path);
    image_ = nullptr;

    // Load image scaled to fit the screen
    image_ = SDLSurfaceUniquePtr{
        SDL_utils::loadImageToFit(filename_, screen.w, screen.h)
    };
    ok_ = (image_ != nullptr);
}

void ImageViewer::onResize()
{
    // Recreate background and reload the image when the window is resized
    image_ = nullptr;
    background_ = nullptr;
    init();
    setPath(std::move(filename_));
}

void ImageViewer::render(const bool focused) const
{
    // Draw background
    SDL_utils::applyPpuScaledSurface(0, 0, background_.get(), screen.surface);

    // Draw centered image
    SDL_utils::applyPpuScaledSurface(
        (screen.actual_w - image_->w) / 2,
        (screen.actual_h - image_->h) / 2,
        image_.get(), screen.surface
    );

    // Draw title bar if enabled
    if (showTitle_) {
        const auto &fonts = CResourceManager::instance().getFonts();

        // Draw background rectangle for title
        SDL_Rect rect = SDL_utils::Rect(0, 0, screen.actual_w, HEADER_H * screen.ppu_y);
        SDL_FillRect(screen.surface, &rect, SDL_MapRGB(screen.surface->format, COLOR_BORDER));

        // Render title text
        SDLSurfaceUniquePtr tmp{
            SDL_utils::renderText(fonts, filename_, Globals::g_colorTextTitle, { COLOR_TITLE_BG })
        };

        SDL_utils::applyPpuScaledSurface(2 * screen.ppu_x, HEADER_PADDING_TOP * screen.ppu_y,
                                         tmp.get(), screen.surface);
    }
}

bool ImageViewer::nextOrPreviousImage(int direction)
{
    while (true) {
        if (direction == -1) {
            if (!panel_->moveCursorUp(1)) return false;

            // Prevent cursor from selecting ".." in image viewer
            if (File_utils::getFileName(panel_->getHighlightedItemFull()) == "..") {
                panel_->moveCursorDown(1);
                return false;
            }
        } else {
            if (!panel_->moveCursorDown(1)) return false;
        }
        std::string new_path = panel_->getHighlightedItemFull();

        constexpr std::size_t kMaxFileSize = 16777216; // = 16 MB
        if (File_utils::getFileSize(new_path) > kMaxFileSize) continue;

        setPath(std::move(new_path));
        if (ok()) return true;
    }
}

// Key press management
bool ImageViewer::keyPress(
    const SDL_Event &event, SDLC_Keycode key, ControllerButton button)
{
    CWindow::keyPress(event, key, button);
    const auto &c = config();
    const auto sym = event.key.keysym.sym;

    // Exit viewer
    if (key == c.key_parent || button == c.gamepad_parent) {
        m_retVal = -1;
        return true;
    }

    // Previous image
    if (key == c.key_up || button == c.gamepad_up ||
        key == c.key_left || button == c.gamepad_left) return actionUp();

    // Next image
    if (key == c.key_down || button == c.gamepad_down ||
        key == c.key_right || button == c.gamepad_right) return actionDown();

    // Toggle title bar
    if (key == c.key_system || button == c.gamepad_system) {
        showTitle_ = !showTitle_;
        return true;
    }

    return false;
}

bool ImageViewer::actionUp() { return nextOrPreviousImage(-1); }
bool ImageViewer::actionDown() { return nextOrPreviousImage(1); }
bool ImageViewer::actionLeft() { return nextOrPreviousImage(-1); }
bool ImageViewer::actionRight() { return nextOrPreviousImage(1); }

bool ImageViewer::keyHold()
{
    const auto &c = config();
    if (tick(c.key_up) || tick(c.key_left)) return actionUp();
    if (tick(c.key_down) || tick(c.key_right)) return actionDown();
    return false;
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
bool CCommander::gamepadHold(SDL_GameController *controller)
{
    const auto &c = config();
    if (tick(controller, c.gamepad_up) || tick(controller, c.gamepad_left)) return actionUp();
    if (tick(controller, c.gamepad_down) || tick(controller, c.gamepad_right)) return actionDown();
    return false;
}
#endif

bool ImageViewer::mouseWheel(int dx, int dy)
{
    CWindow::mouseWheel(dx, dy);
    if (dy < 0) return nextOrPreviousImage(-1);
    if (dy > 0) return nextOrPreviousImage(1);
    return false;
}
