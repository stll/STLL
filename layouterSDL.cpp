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

#include "layouterSDL.h"

#include "layouter.h"
#include "color.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// TODO we need some kind of caching here...

namespace STLL {


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

static uint8_t blend(uint8_t a1, uint8_t a2, uint8_t b)
{
  return a1 + (a2-a1) * b / 255;
}

static void outputGlyph_NONE_Fallback(int sx, int sy, FT_GlyphSlot img, color_c c, SDL_Surface * s)
{
  int stx = (sx+32)/64 + img->bitmap_left;
  int sty = (sy+32)/64 - img->bitmap_top;

  int yp = sty;

  for (int y = 0; y < img->bitmap.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;

      for (int x = 0; x < img->bitmap.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          uint8_t a = (c.a()*img->bitmap.buffer[y*img->bitmap.pitch+x]+128)/255;
          auto p = getpixel(s, xp, yp);

          Uint8 r, g, b;
          SDL_GetRGB(p, s->format, &r, &g, &b);

          r = blend(r, c.r(), a);
          g = blend(g, c.g(), a);
          b = blend(b, c.b(), a);

          putpixel(s, xp, yp, SDL_MapRGBA(s->format, r, g, b, SDL_ALPHA_OPAQUE));

        }
        xp++;
      }
    }
    yp++;
  }
}

static void outputGlyph_Horizontal_Fallback(int sx, int sy, FT_GlyphSlot img, color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img->bitmap_left;
  int sty = sy/64 - img->bitmap_top;
  int stc = (3*sx/64) % 3;

  int yp = sty;

  for (int y = 0; y < img->bitmap.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;
      int col = stc;

      for (int x = 0; x < img->bitmap.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          auto p = getpixel(s, xp, yp);

          Uint8 r, g, b;
          SDL_GetRGB(p, s->format, &r, &g, &b);

          // blend values
          switch (col)
          {
            case 0: r = blend(r, c.r(), (img->bitmap.buffer[y*img->bitmap.pitch+x] * c.a() + 128) / 255); break;
            case 1: g = blend(g, c.g(), (img->bitmap.buffer[y*img->bitmap.pitch+x] * c.a() + 128) / 255); break;
            case 2: b = blend(b, c.b(), (img->bitmap.buffer[y*img->bitmap.pitch+x] * c.a() + 128) / 255); break;
          }

          putpixel(s, xp, yp, SDL_MapRGBA(s->format, r, g, b, SDL_ALPHA_OPAQUE));
        }

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

static void outputGlyph(int sx, int sy, FT_GlyphSlot img, SubPixelArrangement sp, color_c c, SDL_Surface * s)
{
  // check for the right image format
  if (!img) return;
  if (img->format != FT_GLYPH_FORMAT_BITMAP) return;
  if (sp == SUBP_NONE && img->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) return;
  if ((sp == SUBP_RGB || sp == SUBP_BGR) && img->bitmap.pixel_mode != FT_PIXEL_MODE_LCD) return;
  if ((sp == SUBP_RGB_V || sp == SUBP_BGR_V) && img->bitmap.pixel_mode != FT_PIXEL_MODE_LCD_V) return;

  // hub code to decide which function to use for output, there are fast functions
  // for some of the output functions and fallbacks that always work

  switch (sp)
  {
    case SUBP_NONE:
      // no suppixels
      outputGlyph_NONE_Fallback(sx, sy, img, c, s);
      break;

    case SUBP_RGB:

      outputGlyph_Horizontal_Fallback(sx, sy, img, c, s);
      break;
  }
}


void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s, SubPixelArrangement sp)
{
  /* set up rendering via spanners */
  SDL_Rect r;

  /* render */
  for (auto & i : l.data)
  {
    switch (i.command)
    {
      case textLayout_c::commandData::CMD_GLYPH:

        outputGlyph(sx+i.x, sy+i.y, i.font->renderGlyph(i.glyphIndex, sp), sp, i.c, s);
        break;

      case textLayout_c::commandData::CMD_RECT:

        r.x = (i.x+sx+32)/64;
        r.y = (i.y+sy+32)/64;
        r.w = (i.x+sx+i.w+32)/64-r.x;
        r.h = (i.y+sy+i.h+32)/64-r.y;
        SDL_FillRect(s, &r, SDL_MapRGBA(s->format, i.c.r(), i.c.g(), i.c.b(), i.c.a()));
        break;

      case textLayout_c::commandData::CMD_IMAGE:

        // TODO this is just a placeholder for now

        r.x = (i.x+sx+32)/64;
        r.y = (i.y+sy+32)/64;
        r.w = 10;
        r.h = 10;
        SDL_FillRect(s, &r, SDL_MapRGBA(s->format, 255, 255, 255, SDL_ALPHA_OPAQUE));
        break;
    }
  }
}

}
