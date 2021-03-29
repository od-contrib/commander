#ifndef TEXT_VIEWER_H_
#define TEXT_VIEWER_H_

#include <string>

#include "sdl_ptrs.h"
#include "sdl_ttf_multifont.h"
#include "window.h"

class TextViewer : public CWindow {
  public:
    explicit TextViewer(std::string filename);
    virtual ~TextViewer() = default;

    TextViewer(const TextViewer &) = delete;
    TextViewer &operator=(const TextViewer &) = delete;

  private:
    void init();

    void render(const bool focused) const override;
    const bool keyPress(const SDL_Event &event) override;
    const bool keyHold() override;
    bool mouseDown(int button, int x, int y) override;
    void onResize() override;
    bool isFullScreen() const override { return true; }

    // Scroll:
    bool moveUp(unsigned step);
    bool moveDown(unsigned step);
    bool moveLeft();
    bool moveRight();

    const Fonts &fonts_;
    std::string filename_;
    SDLSurfaceUniquePtr background_;
    SDLSurfaceUniquePtr image_;
    SDL_Rect clip_;

    // Text mode:
    std::vector<std::string> lines_;
    std::size_t first_line_;
};

#endif // TEXT_VIEWER_H_
