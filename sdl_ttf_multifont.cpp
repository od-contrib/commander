#include "sdl_ttf_multifont.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

/* This function is incorrect as it only supports BMP but that's all SDL_ttf
 * supports anyway. */
std::vector<std::uint16_t> DecodeUtf8(const std::string &utf8) {
  std::vector<std::uint16_t> result;
  for (std::size_t i = 0; i < utf8.size(); ++i) {
    std::uint16_t ch = static_cast<unsigned char>(utf8[i]);
    if (ch >= 0xF0) {
      ch = static_cast<std::uint16_t>(utf8[i] & 0x07) << 18;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F) << 12;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F) << 6;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F);
    } else if (ch >= 0xE0) {
      ch = static_cast<std::uint16_t>(utf8[i] & 0x0F) << 12;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F) << 6;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F);
    } else if (ch >= 0xC0) {
      ch = static_cast<std::uint16_t>(utf8[i] & 0x1F) << 6;
      ch |= static_cast<std::uint16_t>(utf8[++i] & 0x3F);
    }
    result.push_back(ch);
  }
  result.push_back(0);
  return result;
}

struct Slice {
  std::uint16_t *text;
  std::size_t text_size;
  TTF_Font *font;
};

std::vector<Slice> SplitIntoSlices(
    std::vector<std::uint16_t> &code_points,
    const std::vector<TTF_Font *> &fonts) {
  TTF_Font *prev_font = nullptr;
  std::vector<Slice> slices;
  for (auto &cp : code_points) {
    if (cp == 0) break;
    TTF_Font *cur_font = fonts[0];
    for (auto *font : fonts) {
      if (TTF_GlyphIsProvided(font, cp)) {
        cur_font = font;
        break;
      }
    }
    if (cur_font == prev_font) {
      ++slices.back().text_size;
    } else {
      slices.push_back(Slice{&cp, 1, cur_font});
      prev_font = cur_font;
    }
  }
  return slices;
}

}  // namespace

SDL_Surface *TTFMultiFont_RenderUTF8_Shaded(
    const std::vector<TTF_Font *> &fonts, const std::string &text, SDL_Color fg,
    SDL_Color bg) {
  if (fonts.size() == 1)
    return TTF_RenderUTF8_Shaded(fonts[0], text.c_str(), fg, bg);

  auto code_points = DecodeUtf8(text);
  const std::vector<Slice> slices = SplitIntoSlices(code_points, fonts);

  if (slices.empty()) return nullptr;
  if (slices.size() == 1)
    return TTF_RenderUNICODE_Shaded(slices[0].font, slices[0].text, fg, bg);

  std::vector<SDL_Surface *> surfaces;
  surfaces.reserve(slices.size());

  // SDL_ttf API requires a 0-terminated buffer.
  // To avoid excessive copying, we iterate in-reverse and modify the buffer
  // in-place.
  for (std::size_t i = slices.size(); i > 0; --i) {
    auto &slice = slices[i - 1];
    slice.text[slice.text_size] = 0;
    SDL_Surface *surface =
        TTF_RenderUNICODE_Shaded(slice.font, slice.text, fg, bg);
    if (surface == nullptr) {
      std::cerr << "TTFMultiFont_RenderUTF8_Shaded error: " << SDL_GetError()
                << std::endl;
      SDL_ClearError();
      continue;
    }
    surfaces.push_back(surface);
  }

  int width = 0;
  int height = 0;
  for (auto *surface : surfaces) {
    width += surface->w;
    height = std::max(surface->h, height);
  }

  SDL_Surface *result =
      SDL_AllocSurface(SDL_SWSURFACE, width, height, 8, 0, 0, 0, 0);
  if (result == nullptr) return nullptr;
  SDL_SetPalette(result, SDL_LOGPAL, surfaces[0]->format->palette->colors, 0,
                 surfaces[0]->format->palette->ncolors);
  decltype(SDL_Rect().x) x = 0;
  for (std::size_t i = surfaces.size(); i > 0; --i) {
    auto *surface = surfaces[i - 1];
    SDL_Rect dst_rect = {
        x, static_cast<decltype(SDL_Rect().y)>(height - surface->h),
        static_cast<decltype(SDL_Rect().w)>(surface->w),
        static_cast<decltype(SDL_Rect().h)>(surface->h)};
    SDL_BlitSurface(surface, nullptr, result, &dst_rect);
    x += static_cast<decltype(SDL_Rect().w)>(surface->w);
  }

  for (auto *surface : surfaces) SDL_FreeSurface(surface);

  return result;
}
