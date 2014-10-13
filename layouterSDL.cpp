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

#include <unordered_map>

namespace STLL {

// TODO properly handle it, when FreeType returns an bitmap format that is not supported

class GlyphData_c
{
  public:
    int32_t left;
    int32_t top;
    int32_t rows;
    int32_t width;
    int32_t pitch;
    uint8_t * buffer;
    uint32_t lastUse;

    GlyphData_c(FT_GlyphSlotRec_ * ft) :
      left(ft->bitmap_left), top(ft->bitmap_top), rows(ft->bitmap.rows),
      width(ft->bitmap.width), pitch(ft->bitmap.pitch), buffer(0)
    {
      buffer = new uint8_t [rows*pitch];
      memcpy(buffer, ft->bitmap.buffer, rows*pitch);
    }

    GlyphData_c(GlyphData_c && g) :
      left(g.left), top(g.top), rows(g.rows),
      width(g.width), pitch(g.pitch), buffer(0)
    {
      buffer = g.buffer;
      g.buffer = 0;
    }

    ~GlyphData_c(void)
    {
      if (buffer)
        delete [] buffer;
    }
};

static std::unordered_map<GlyphKey_c, GlyphData_c> glyphCache;
static uint32_t useCounter = 0;

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

static void outputGlyph_NONE_Fallback(int sx, int sy, const GlyphData_c & img, color_c c, SDL_Surface * s)
{
  int stx = (sx+32)/64 + img.left;
  int sty = (sy+32)/64 - img.top;

  int yp = sty;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;

      for (int x = 0; x < img.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          uint8_t a = (c.a()*img.buffer[y*img.pitch+x]+128)/255;
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

static void outputGlyph_NONE_RGBx(int sx, int sy, const GlyphData_c & img, color_c c, SDL_Surface * s)
{
  int stx = (sx+32)/64 + img.left;
  int sty = (sy+32)/64 - img.top;

  int yp = sty;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;

      uint8_t * dst = (uint8_t*)s->pixels + yp * s->pitch + 4*xp;
      uint8_t * src = img.buffer + y*img.pitch;

      for (int x = 0; x < img.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          uint8_t a = (c.a()*img.buffer[y*img.pitch+x]+128)/255;

          *dst = blend(*dst, c.b(), a); dst++;
          *dst = blend(*dst, c.g(), a); dst++;
          *dst = blend(*dst, c.r(), a); dst++;
          dst++;
        }
        xp++;
      }
    }
    yp++;
  }
}

static void outputGlyph_HorizontalRGB_Fallback(int sx, int sy, const GlyphData_c & img, color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = sy/64 - img.top;
  int stc = (3*sx/64) % 3;

  int yp = sty;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;
      int col = stc;

      for (int x = 0; x < img.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          auto p = getpixel(s, xp, yp);

          Uint8 r, g, b;
          SDL_GetRGB(p, s->format, &r, &g, &b);

          // blend values
          switch (col)
          {
            case 0: r = blend(r, c.r(), (img.buffer[y*img.pitch+x] * c.a() + 128) / 255); break;
            case 1: g = blend(g, c.g(), (img.buffer[y*img.pitch+x] * c.a() + 128) / 255); break;
            case 2: b = blend(b, c.b(), (img.buffer[y*img.pitch+x] * c.a() + 128) / 255); break;
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

static void outputGlyph_HorizontalRGB_RGBx(int sx, int sy, const GlyphData_c & img, color_c c, SDL_Surface * s)
{
  int stx = sx/64 + img.left;
  int sty = (sy+32)/64 - img.top;
  int stc = (3*sx/64) % 3;

  int yp = sty;

  assert(img.width % 3 == 0);

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < s->h)
    {
      int xp = stx;
      int col = stc;

      uint8_t * dst = (uint8_t*)s->pixels + yp * s->pitch + 4*xp + 2 - col;
      uint8_t * src = img.buffer + y*img.pitch;

      for (int x = 0; x < img.width; x++)
      {
        if (xp >= 0 && xp < s->w )
        {
          switch(col)
          {
            case 0: *dst = blend(*dst, c.r(), (*src*c.a()+128)/255); break;
            case 1: *dst = blend(*dst, c.g(), (*src*c.a()+128)/255); break;
            case 2: *dst = blend(*dst, c.b(), (*src*c.a()+128)/255); break;
          }
        }

        col++;
        dst--;
        src++;
        if (col >= 3)
        {
          col = 0;
          xp++;
          dst+=7;
        }
      }
    }
    yp++;
  }
}

static bool isRGBxSurface(SDL_Surface * s)
{
  auto f = s->format;

  return f->BytesPerPixel == 4 && f->Rmask == 0xFF0000 && f->Gmask == 0xFF00 && f->Bmask == 0xFF;
}

static void outputGlyph(int sx, int sy, const GlyphData_c & img, SubPixelArrangement sp, color_c c, SDL_Surface * s)
{
  // check for the right image format

  // hub code to decide which function to use for output, there are fast functions
  // for some of the output functions and fallbacks that always work

  switch (sp)
  {
    case SUBP_NONE:
      // no suppixels
      if (isRGBxSurface(s))
        outputGlyph_NONE_RGBx(sx, sy, img, c, s);
      else
        outputGlyph_NONE_Fallback(sx, sy, img, c, s);
      break;

    case SUBP_RGB:

      if (isRGBxSurface(s))
        outputGlyph_HorizontalRGB_RGBx(sx, sy, img, c, s);
      else
        outputGlyph_HorizontalRGB_Fallback(sx, sy, img, c, s);
      break;
  }
}

static GlyphData_c & getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp)
{
  GlyphKey_c k(face, glyph, sp);

  auto i = glyphCache.find(k);

  if (i == glyphCache.end())
  {
    auto g = face->renderGlyph(glyph, sp);

    assert(g->format == FT_GLYPH_FORMAT_BITMAP);
    assert( sp != SUBP_NONE                       || g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
    assert((sp != SUBP_RGB && sp != SUBP_BGR)     || g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD);
    assert((sp != SUBP_RGB_V && sp != SUBP_BGR_V) || g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V);

    i = glyphCache.insert(std::make_pair(k, std::move(GlyphData_c(g)))).first;
  }

  i->second.lastUse = useCounter;
  useCounter++;

  return i->second;
}


void showLayoutSDL(const TextLayout_c & l, int sx, int sy, SDL_Surface * s,
                   SubPixelArrangement sp, imageDrawerSDL_c * images)
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

  /* render */
  for (auto & i : l.getData())
  {
    switch (i.command)
    {
      case CommandData_c::CMD_GLYPH:
        outputGlyph(sx+i.x, sy+i.y, getGlyph(i.font, i.glyphIndex, sp), sp, i.c, s);
        break;

      case CommandData_c::CMD_RECT:
        r.x = (i.x+sx+32)/64;
        r.y = (i.y+sy+32)/64;
        r.w = (i.x+sx+i.w+32)/64-r.x;
        r.h = (i.y+sy+i.h+32)/64-r.y;
        SDL_FillRect(s, &r, SDL_MapRGBA(s->format, i.c.r(), i.c.g(), i.c.b(), i.c.a()));
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
  if (num == 0)
  {
    glyphCache.clear();
  }
  else if (num < glyphCache.size())
  {
    // find elements that are oldest in the cache
    std::vector<std::unordered_map<GlyphKey_c, GlyphData_c>::iterator> entries;

    for (auto i = glyphCache.begin(); i != glyphCache.end(); ++i)
      entries.push_back(i);

    std::sort(entries.begin(), entries.end(),
              [] (const std::unordered_map<GlyphKey_c, GlyphData_c>::iterator & a,
                  const std::unordered_map<GlyphKey_c, GlyphData_c>::iterator & b) {
      return a->second.lastUse < b->second.lastUse;
    });

    size_t toDel = glyphCache.size() - num;

    for (size_t i = 0; i < toDel; i++)
      glyphCache.erase(entries[i]);
  }
}

}
