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

#include <ft2build.h>
#include FT_FREETYPE_H

// TODO properly handle it, when FreeType returns an bitmap format that is not supported


namespace STLL
{
// lookup tables for gamma correct output
// gamma contains the gamma value that the lookup tables are currently
// set up for
// GAMMA_SCALE is a scaling factor for limit calculation errors, the bigger
// the more exact the output will be, but the bigger the 2nd lookup has to
// be, and then 2 lookup tables for forward and inverse correction
// the tables are updated when showLayout is called with a new gamma value
static uint8_t gamma = 0;
#define GAMMA_SCALE 8
static uint16_t gammaFor[256] = {};
static uint16_t gammaInv[256*GAMMA_SCALE] = {};

// a simple get pixel function for the fallback render methods
static Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
    case 1:
      return *p;
      break;

    case 2:
      return *(Uint16 *)p;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4:
      return *(Uint32 *)p;
      break;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
}

// a simple put pixel function for the fallback render methods
static void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
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

// linear blending of 2 values
static uint8_t blend(uint8_t a1, uint8_t a2, uint16_t b1, uint16_t b2 = 0, uint8_t c = 0)
{
  // the uncorrected blending function would look like this:
  // return a1 + (a2-a1) * b / 255;
  // but that results in wrong values when the target has a gamma
  // corrected output (e.g. display surfaces for sRGB monitors)
  // so we need to correct gamma forward on a1 and inverse on the output
  // the target colour is already corrected
  int b = (int)b1 + ((int)b2-(int)b1)*c/64;

  int d1 = gammaFor[a1];
  int d2 = a2*GAMMA_SCALE;

  int out = d1 + (d2-d1)*b/(255*255);

  return gammaInv[out];
}

// the fallback glyph rendering without sub-pixel output. This should work on every surface
// independent of its format
static void outputGlyph_NONE_Fallback(int sx, int sy, const internal::PaintData_c & img, Color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = (sy+32)/64 - img.top;
  int stb = sx % 64;

  int yp = sty;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;

      uint16_t a = 0;
      uint16_t aprev = 0;

      for (int x = 0; x <= img.width; x++)
      {
        a = c.a()*img.buffer[y*img.pitch+x];

        if (xp >= 0 && xp < s->w )
        {
          auto p = getpixel(s, xp, yp);

          Uint8 r, g, b;
          SDL_GetRGB(p, s->format, &r, &g, &b);

          r = blend(r, c.r(), a, aprev, stb);
          g = blend(g, c.g(), a, aprev, stb);
          b = blend(b, c.b(), a, aprev, stb);

          putpixel(s, xp, yp, SDL_MapRGBA(s->format, r, g, b, SDL_ALPHA_OPAQUE));
        }

        aprev = a;
        xp++;
      }
    }
    yp++;
  }
}

// an optimized glyph rendering function for not sub-pixel output. Optimized for surfaces with
// 32 bits per pixel in RGBx order, x meaning one unused byte
static void outputGlyph_NONE_RGBx(int sx, int sy, const internal::PaintData_c & img, Color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = (sy+32)/64 - img.top;
  int stb = sx % 64;

  int yp = sty;

  if (stx < 0 || stx + img.width >= s->w)
  {
    outputGlyph_NONE_Fallback(sx, sy, img, c, s);
    return;
  }

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      uint8_t * dst = (uint8_t*)s->pixels + yp * s->pitch + 4*stx;
      const uint8_t * src = img.getBuffer() + y*img.pitch;

      uint16_t a = 0;
      uint16_t aprev = 0;

      for (int x = 0; x <= img.width; x++)
      {
        a = *src*c.a();

        *dst = blend(*dst, c.b(), a, aprev, stb); dst++;
        *dst = blend(*dst, c.g(), a, aprev, stb); dst++;
        *dst = blend(*dst, c.r(), a, aprev, stb); dst++;
        dst++;
        src++;

        aprev = a;
      }
    }
    yp++;
  }
}

// a fallback output function with sub-pixel glyph placement, should work on all
// target surface formats
static void outputGlyph_HorizontalRGB_Fallback(int sx, int sy, const internal::PaintData_c & img, Color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = sy/64 - img.top;
  int stc = (3*sx/64) % 3;
  int stb = (3*sx) % 64;

  int yp = sty;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;
      int col = stc;

      uint16_t a = 0;
      uint16_t aprev = 0;

      for (int x = 0; x <= img.width; x++)
      {
        a = img.buffer[y*img.pitch+x]*c.a();

        if (xp >= 0 && xp < s->w )
        {
          auto p = getpixel(s, xp, yp);

          Uint8 r, g, b;
          SDL_GetRGB(p, s->format, &r, &g, &b);

          // blend values
          switch (col)
          {
            case 0: r = blend(r, c.r(), a, aprev, stb); break;
            case 1: g = blend(g, c.g(), a, aprev, stb); break;
            case 2: b = blend(b, c.b(), a, aprev, stb); break;
          }

          putpixel(s, xp, yp, SDL_MapRGBA(s->format, r, g, b, SDL_ALPHA_OPAQUE));
        }

        aprev = a;

        col++;
        if (col >= 3)
        {
          col = 0;
          xp++;
        }
      }
    }
    yp++;
  }
}

// sub-pixel glyph output optimized for 32 bit surfaces with red green blue unused order
static void outputGlyph_HorizontalRGB_RGBx(int sx, int sy, const internal::PaintData_c & img, Color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = (sy+32)/64 - img.top;
  int stc = (3*sx/64) % 3;
  int stb = (3*sx) % 64;

  int yp = sty;

  if (stx < 0 || stx + img.width/3 >= s->w || img.width < 6)
  {
    outputGlyph_HorizontalRGB_Fallback(sx, sy, img, c, s);
    return;
  }

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      uint8_t * dst = (uint8_t*)s->pixels + yp * s->pitch + 4*stx + 2 - stc;
      const uint8_t * src = img.getBuffer() + y*img.pitch;

      uint16_t a = 0;
      uint16_t aprev = 0;

      // output single sub-pixel elements until we reach a proper pixel border
      switch (stc)
      {
        case 1: a = *src*c.a(); *dst = blend(*dst, c.g(), a, aprev, stb); aprev = a; src++; dst--;
        case 2: a = *src*c.a(); *dst = blend(*dst, c.b(), a, aprev, stb); aprev = a; src++; dst += 6;
      }

      int x = img.width/3;

      // output the remainder in a more efficient loop in whole pixel sections
      // this assumes that the image width is a bit more than the width declared in img.width
      // and that the additional columns are empty
      while (x > 0)
      {
        a = *src*c.a(); *dst = blend(*dst, c.r(), a, aprev, stb); aprev = a; src++; dst--;
        a = *src*c.a(); *dst = blend(*dst, c.g(), a, aprev, stb); aprev = a; src++; dst--;
        a = *src*c.a(); *dst = blend(*dst, c.b(), a, aprev, stb); aprev = a; src++; dst += 6;
        x --;
      }
    }
    yp++;
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
  if (f->BytesPerPixel == 4 && f->Rmask == 0xFF0000 && f->Gmask == 0xFF00 && f->Bmask == 0xFF)
    return 1;
#endif

  // default format
  return 0;
}

static void outputGlyph(int sx, int sy, const internal::PaintData_c & img, SubPixelArrangement sp, Color_c c, SDL_Surface * s)
{
  // check for the right image format

  // hub code to decide which function to use for output, there are fast functions
  // for some of the output functions and fallbacks that always work
  // the fallbacks are also done very exact with clipping and such, while the
  // fast functions might not do so, but only check if clipping is required and then
  // forward to the fallback function

  switch (calcFormatID(getSurfaceFormat(s), sp))
  {
    default: // use no subpixel and no optimisation as default... should not happen though
    case calcFormatID(0, SUBP_NONE): outputGlyph_NONE_Fallback(sx, sy, img, c, s);          break;
    case calcFormatID(1, SUBP_NONE): outputGlyph_NONE_RGBx(sx, sy, img, c, s);              break;
    case calcFormatID(0, SUBP_RGB):  outputGlyph_HorizontalRGB_Fallback(sx, sy, img, c, s); break;
    case calcFormatID(1, SUBP_RGB):  outputGlyph_HorizontalRGB_RGBx(sx, sy, img, c, s);     break;
  }
}

static Color_c gammaColor(Color_c c)
{
  return Color_c(
    gammaFor[c.r()]/GAMMA_SCALE,
    gammaFor[c.g()]/GAMMA_SCALE,
    gammaFor[c.b()]/GAMMA_SCALE,
    c.a());
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

  if (g != gamma)
  {
    gamma = g;

    for (int i = 0; i < 256; i++)
      gammaFor[i] = (256*GAMMA_SCALE-1)*pow(i/255.0, g*0.1);

    for (int i = 0; i < 256*GAMMA_SCALE; i++)
      gammaInv[i] = 255*pow(i/(1.0*256*GAMMA_SCALE-1), 10.0/g);
  }

  /* render */
  for (auto & i : l.getData())
  {
    switch (i.command)
    {
      case CommandData_c::CMD_GLYPH:
        outputGlyph(sx+i.x, sy+i.y, internal::cache_single_getGlyph(i.font, i.glyphIndex, sp, i.blurr), sp, gammaColor(i.c), s);
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
          outputGlyph(sx+i.x, sy+i.y, internal::cache_single_getRect(i.w, i.h, sp, i.blurr), sp, gammaColor(i.c), s);
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
