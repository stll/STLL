/*
 * STLL Simple Text Layouting Library
 *
 * STLL is the legal property of its developers, whose
 * names are listed in the COPYRIGHT file, which is included
 * within the source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stll/output_SDL.h>

#include <stll/layouter.h>
#include <stll/color.h>

#include "cache_single.h"
#include "gamma.h"
#include "blitter.h"



namespace STLL
{

static Gamma_c<8> gamma;

// a simple get pixel function for the fallback render methods
std::tuple<uint8_t, uint8_t, uint8_t> getpixel(const uint8_t * p, const SDL_PixelFormat * f)
{
  uint32_t val;

  switch(f->BytesPerPixel) {
    case 1: val = *p;
      break;

    case 2: val = *(Uint16 *)p;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        val = p[0] << 16 | p[1] << 8 | p[2];
      else
        val = p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4: val = *(Uint32 *)p;
      break;

    default:
      val = 0;       /* shouldn't happen, but avoids warnings */
      break;
  }

  Uint8 r, g, b;
  SDL_GetRGB(val, f, &r, &g, &b);

  return std::make_tuple(r, g, b);
}

// a simple put pixel function for the fallback render methods
void putpixel(uint8_t * p, uint8_t r, uint8_t g, uint8_t b, const SDL_PixelFormat * f)
{
  uint32_t pixel = SDL_MapRGB(f, r, g, b);

  switch(f->BytesPerPixel) {
    case 1:
      *p = pixel;
      break;

    case 2:
      *(Uint16 *)p = pixel;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
      break;

    case 4:
      *(Uint32 *)p = pixel;
      break;
  }
}

constexpr static int calcFormatID(int format, SubPixelArrangement sp)
{
  return (int)(sp)+format*8;
}

static int getSurfaceFormat(SDL_Surface * s)
{
  auto f = s->format;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  if (f->Rmask == 0xFF0000 && f->Gmask == 0xFF00 && f->Bmask == 0xFF)
    return 1;
#endif

  // default format
  return 0;
}

static void outputGlyph(int sx, int sy, const internal::PaintData_c & img, SubPixelArrangement sp, Color_c c, SDL_Surface * s)
{
  // check for the right image format

  // hub code to decide which function to use for output, there are fast functions
  // for some of the output functions and fallbacks that always work but use relatively slow
  // pixel read and write routines
  // if you need additional surface formats add them here (e.g other 3 or 4 Byte formats with
  // a different byte order
  // as I don't know what is a useful set to support I only add the one format I I know of: mine

  switch (calcFormatID(getSurfaceFormat(s), sp))
  {
    default: // use no subpixel and no optimisation as default... should not happen though
    case calcFormatID(0, SUBP_NONE):
      outputGlyph_NONE(
        sx, sy, img, c, (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
        [s](const uint8_t * p) -> auto { return getpixel(p, s->format); },
        [s](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { putpixel(p, r, g, b, s->format); },
        gamma);
      break;
    case calcFormatID(1, SUBP_NONE):
      outputGlyph_NONE(
        sx, sy, img, c, (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
        [s](const uint8_t * p) -> auto { return std::make_tuple(p[2], p[1], p[0]); },
        [s](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { p[2] = r; p[1] = g; p[0] = b; },
        gamma);
      break;
    case calcFormatID(0, SUBP_RGB):
      outputGlyph_HorizontalRGB(
        sx, sy, img, c, (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
        [s](const uint8_t * p) -> auto { return getpixel(p, s->format); },
        [s](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { putpixel(p, r, g, b, s->format); },
        gamma);
      break;
    case calcFormatID(1, SUBP_RGB):
      outputGlyph_HorizontalRGB(
        sx, sy, img, c, (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
        [s](const uint8_t * p) -> auto { return std::make_tuple(p[2], p[1], p[0]); },
        [s](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { p[2] = r; p[1] = g; p[0] = b; },
        gamma);
      break;
  }
}



void showLayoutSDL(const TextLayout_c & l, int sx, int sy, SDL_Surface * s,
                   SubPixelArrangement sp, ImageDrawerSDL_c * images, uint8_t g)
{
  /* set up rendering via spanners */
  SDL_Rect r;

#ifndef NDEBUG
  int cnt = 1;
  for (auto & i : l.links)
  {
    int c2 = 10;
    for (auto & j : i.areas)
    {
      r.x = (j.x+sx+32)/64;
      r.y = (j.y+sy+32)/64;
      r.w = (j.w+63)/64;
      r.h = (j.h+63)/64;
      SDL_FillRect(s, &r, SDL_MapRGBA(s->format, 10*(uint8_t)(123*cnt)/c2, 10*(uint8_t)(183*cnt)/c2, 10*(uint8_t)(421*cnt)/c2, 128));

      c2++;
    }
    cnt++;
  }
#endif

  gamma.setGamma(g);

  /* render */
  for (auto & i : l.getData())
  {
    switch (i.command)
    {
      case CommandData_c::CMD_GLYPH:
        outputGlyph(sx+i.x, sy+i.y, internal::cache_single_getGlyph(i.font, i.glyphIndex, sp, i.blurr), sp, gamma.forward(i.c), s);
        break;

      case CommandData_c::CMD_RECT:
        if (i.blurr == 0)
        {
          r.x = (i.x+sx+32)/64;
          r.y = (i.y+sy+32)/64;
          r.w = (i.x+sx+i.w+32)/64-r.x;
          r.h = (i.y+sy+i.h+32)/64-r.y;
          SDL_FillRect(s, &r, SDL_MapRGBA(s->format, i.c.r(), i.c.g(), i.c.b(), i.c.a()));
        }
        else
        {
          outputGlyph(sx+i.x, sy+i.y, internal::cache_single_getRect(i.w, i.h, sp, i.blurr), sp, gamma.forward(i.c), s);
        }
        break;

      case CommandData_c::CMD_IMAGE:
        if (images)
          images->draw(i.x+sx, i.y+sy, i.w, i.h, s, i.imageURL);
        break;
    }
  }
}

void trimSDLFontCache(size_t num)
{
  internal::cache_single_trim(num);
}

}
