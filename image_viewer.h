#ifndef IMAGE_VIEWER_H_
#define IMAGE_VIEWER_H_

#include <cstddef>
#include <string>

#include "panel.h"
#include "sdl_ptrs.h"
#include "window.h"

class ImageViewer : public CWindow {
  public:
    explicit ImageViewer(CPanel *panel);
    virtual ~ImageViewer() = default;

    ImageViewer(const ImageViewer &) = delete;
    ImageViewer &operator=(const ImageViewer &) = delete;

    bool ok() const { return ok_; }

  private:
    void setPath(std::string &&path);
    void init();
    void render(const bool focused) const override;

    // Key press management
    bool keyPress(const SDL_Event &event, SDLC_Keycode key,
        ControllerButton button) override;

    // Key hold management
    bool keyHold() override;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    bool gamepadHold(SDL_GameController *controller) override;
#endif

    bool mouseWheel(int dx, int dy) override;
    void onResize() override;
    bool isFullScreen() const override { return true; }

    bool nextOrPreviousImage(int direction);

    SDLSurfaceUniquePtr background_;
    SDLSurfaceUniquePtr image_;
    bool ok_;
    CPanel *panel_;
    std::string filename_;

    bool showTitle_;

    // Repeated actions
    bool actionUp();
    bool actionDown();
    bool actionLeft();
    bool actionRight();
};

#endif // IMAGE_VIEWER_H_
