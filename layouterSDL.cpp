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
#include "blurr.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>

namespace STLL {

// TODO properly handle it, when FreeType returns an bitmap format that is not supported

// encapsulation for an object to paint it contains the data for the alpha value
// of an object to paint. It is used to store information about single glyphs or
// single rectangles to draw
class PaintData_c
{
  private:

    // copy the buffer and apply a blurr, if needed, when buffer pointer
    // is nullptr, then the whole buffer is assumed to be completely filled with 255 value (used
    // for rectangles)
    void blurrInit(uint8_t * buf_dat, uint16_t buf_pitch, uint16_t buf_rows, uint16_t blurr, SubPixelArrangement sp)
    {
      // create a buffer that is big enough to hold the complete blurred image
      // and has an additional number of columns on the right so that no additional
      // checks are needed for drawing for the last column
      // so the pitch of the buffered image is always 1 or 2 columns wider than the image width (depending on suppixel)
      int blurrh = 1;
      int blurrw = 1;
      int addc = 1;
      int ts = blurr/32;
      int ls = blurr/32;

      left -= ls;
      top += ts;

      switch (sp)
      {
        default:
        case SUBP_NONE:
          break;

        case SUBP_RGB:
        case SUBP_BGR:
          blurrw *= 3;
          ls *= 3;
          addc *= 3;
          break;
      }

      width += 2*ls;
      pitch += 2*ls;
      rows += 2*ts;

      // make the pitxh bigger
      pitch += addc;

      buffer = std::make_unique<uint8_t[]>(rows*pitch);
      memset(buffer.get(), 0, rows*pitch);

      // copy image into the centre of the enlarged buffer
      for (int i = 0; i < buf_rows; i++)
      {
        if (buf_dat)
          memcpy(buffer.get()+(i+ts)*pitch+ls, buf_dat+i*(buf_pitch), buf_pitch);
        else
          memset(buffer.get()+(i+ts)*pitch+ls, 255, buf_pitch);
      }

      if (blurr > 0)
      {
        gaussBlur(buffer.get(), pitch, rows, blurr/64.0, blurrw, blurrh);
      }
    }

  public:
    int32_t left;  // position of the top left corner relative to the baseposition of the image
    int32_t top;
    int32_t rows;  // hight of image
    int32_t width; // width of image
    int32_t pitch; // number of bytes per line of image, guaranteed to be at least 1 or 2 bigger than width
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t lastUse;

    // create from glyph data
    PaintData_c(FT_GlyphSlotRec_ * ft, uint16_t blurr, SubPixelArrangement sp) :
      left(ft->bitmap_left), top(ft->bitmap_top), rows(ft->bitmap.rows),
      width(ft->bitmap.width), pitch(ft->bitmap.pitch)
    {
      blurrInit(ft->bitmap.buffer, ft->bitmap.pitch, ft->bitmap.rows, blurr, sp);
    }

    // create rectangle data
    PaintData_c(uint16_t _pitch, uint16_t _rows, uint16_t blurr, SubPixelArrangement sp) :
      left(0), top(0), rows(_rows), width(_pitch), pitch(_pitch)
    {
      blurrInit(0, _pitch, _rows, blurr, sp);
    }

    const uint8_t * getBuffer(void) const { return buffer.get(); }
};

// hash key similar to the GlyphKey_c
class RectKey_c
{
  public:
    int w;
    int h;
    int blurr;
    SubPixelArrangement sp;

    RectKey_c(int wi, int hi, int b, SubPixelArrangement s) : w(wi), h(hi), blurr(b), sp(s) {}

    bool operator==(const RectKey_c & a) const
    {
      return w == a.w && h == a.h && sp == a.sp && blurr == a.blurr;
    }

};

}

namespace std {

template <>
class hash<STLL::RectKey_c>
{
  public :
  size_t operator()(const STLL::RectKey_c & name ) const
  {
    return (size_t)name.w * (size_t)name.h * (size_t)name.blurr * (size_t)(name.sp);
  }
};

};

namespace STLL
{

// our glyph cache with all the rendered glyphs
static std::unordered_map<GlyphKey_c, PaintData_c> glyphCache;
static std::unordered_map<RectKey_c, PaintData_c> rectCache;

// each time we access a glyph from the cache we increase this number
// and write the value into the lastUse field of the rendered glyph
// that is how we can find out glyphs that were not used the longest time
static uint32_t useCounter = 0;

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
static void outputGlyph_NONE_Fallback(int sx, int sy, const PaintData_c & img, Color_c c, SDL_Surface * s)
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
static void outputGlyph_NONE_RGBx(int sx, int sy, const PaintData_c & img, Color_c c, SDL_Surface * s)
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
static void outputGlyph_HorizontalRGB_Fallback(int sx, int sy, const PaintData_c & img, Color_c c, SDL_Surface * s)
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
static void outputGlyph_HorizontalRGB_RGBx(int sx, int sy, const PaintData_c & img, Color_c c, SDL_Surface * s)
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

static void outputGlyph(int sx, int sy, const PaintData_c & img, SubPixelArrangement sp, Color_c c, SDL_Surface * s)
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

// get the glyph from the cache, or render new using FreeType
static PaintData_c & getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp, uint16_t blurr)
{
  GlyphKey_c k(face, glyph, sp, blurr);

  auto i = glyphCache.find(k);

  if (i == glyphCache.end())
  {
    auto g = face->renderGlyph(glyph, sp);

    assert(g->format == FT_GLYPH_FORMAT_BITMAP);
    assert( sp != SUBP_NONE                       || g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
    assert((sp != SUBP_RGB && sp != SUBP_BGR)     || g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD);
    assert((sp != SUBP_RGB_V && sp != SUBP_BGR_V) || g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V);

    i = glyphCache.insert(std::make_pair(k, std::move(PaintData_c(g, blurr, sp)))).first;
  }

  i->second.lastUse = useCounter;
  useCounter++;

  return i->second;
}

static PaintData_c & getRect(int w, int h, SubPixelArrangement sp, uint16_t blurr)
{
  RectKey_c k(w, h, blurr, sp);

  auto i = rectCache.find(k);

  if (i == rectCache.end())
  {
    int rw = (w+32)/64;
    int rh = (h+32)/64;

    switch (sp)
    {
      default:
      case SUBP_NONE:
        break;

      case SUBP_RGB:
        rw = (3*w+32)/64;
        break;
    }

    i = rectCache.insert(std::make_pair(k, std::move(PaintData_c(rw, rh, blurr, sp)))).first;
  }

  return i->second;
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
                   SubPixelArrangement sp, imageDrawerSDL_c * images, uint8_t g)
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
        outputGlyph(sx+i.x, sy+i.y, getGlyph(i.font, i.glyphIndex, sp, i.blurr), sp, gammaColor(i.c), s);
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
          outputGlyph(sx+i.x, sy+i.y, getRect(i.w, i.h, sp, i.blurr), sp, gammaColor(i.c), s);
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
  if (num == 0)
  {
    glyphCache.clear();
  }
  else if (num < glyphCache.size())
  {
    // find elements that are oldest in the cache
    std::vector<std::unordered_map<GlyphKey_c, PaintData_c>::iterator> entries;

    for (auto i = glyphCache.begin(); i != glyphCache.end(); ++i)
      entries.push_back(i);

    std::sort(entries.begin(), entries.end(),
              [] (const std::unordered_map<GlyphKey_c, PaintData_c>::iterator & a,
                  const std::unordered_map<GlyphKey_c, PaintData_c>::iterator & b) {
      return a->second.lastUse < b->second.lastUse;
    });

    size_t toDel = glyphCache.size() - num;

    for (size_t i = 0; i < toDel; i++)
      glyphCache.erase(entries[i]);
  }

  // we always clear out the complete rectangle cache, they are cheap to redo when necessary
  rectCache.clear();
}

}
