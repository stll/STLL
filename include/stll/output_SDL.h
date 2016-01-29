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
#ifndef STLL_LAYOUTER_SDL
#define STLL_LAYOUTER_SDL

/** \file
 *  \brief SDL output driver
 */

#include "layouterFont.h"
#include "layouter.h"
#include "color.h"

#include "internal/glyphCache.h"
#include "internal/blitter.h"
#include "internal/gamma.h"

#include <SDL.h>

namespace STLL {

/** \brief a class to output layouts using SDL
 *
 * To output layouts using this class, create an object of it and then
 * use the showLayout Function to output the layout.
 *
 * \tparam G the gamma calculation class to use... normally you don't need to change this, keep the default
 */
template <class G = internal::Gamma_c<>>
class showSDL
{
  private:
    G g;
    internal::GlyphCache_c cache;

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

    void outputGlyph(int sx, int sy, const internal::PaintData_c & img, SubPixelArrangement sp, Color_c c, SDL_Surface * s)
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
            [s, this](const uint8_t * p) -> auto { return getpixel(p, s->format); },
            [s, this](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { putpixel(p, r, g, b, s->format); },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
        case calcFormatID(1, SUBP_NONE):
          outputGlyph_NONE(
            sx, sy, img, c, (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
            [](const uint8_t * p) -> auto { return std::make_tuple(p[2], p[1], p[0]); },
            [](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { p[2] = r; p[1] = g; p[0] = b; },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
        case calcFormatID(0, SUBP_RGB):
          outputGlyph_HorizontalRGB(
            sx, sy, img, c.r(), c.g(), c.b(), c.a(), (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
            [s, this](const uint8_t * p) -> auto { return getpixel(p, s->format); },
            [s, this](uint8_t * p, uint8_t r, uint8_t g, uint8_t b) -> void { putpixel(p, r, g, b, s->format); },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
        case calcFormatID(1, SUBP_RGB):
          outputGlyph_HorizontalRGB(
            sx, sy, img, c.r(), c.g(), c.b(), c.a(), (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
            [](const uint8_t * p) -> auto { return std::make_tuple(p[2], p[1], p[0]); },
            [](uint8_t * p, uint8_t sp1, uint8_t sp2, uint8_t sp3) -> void { p[2] = sp1; p[1] = sp2; p[0] = sp3; },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
        case calcFormatID(0, SUBP_BGR):
          outputGlyph_HorizontalRGB(
            sx, sy, img, c.b(), c.g(), c.r(), c.a(), (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
            [s, this](const uint8_t * p) -> auto { auto t = getpixel(p, s->format); return std::make_tuple(std::get<2>(t), std::get<1>(t), std::get<0>(t)); },
            [s, this](uint8_t * p, uint8_t sp1, uint8_t sp2, uint8_t sp3) -> void { putpixel(p, sp3, sp2, sp1, s->format); },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
        case calcFormatID(1, SUBP_BGR):
          outputGlyph_HorizontalRGB(
            sx, sy, img, c.b(), c.g(), c.r(), c.a(), (uint8_t*)s->pixels, s->pitch, s->format->BytesPerPixel, s->w, s->h,
            [](const uint8_t * p) -> auto { return std::make_tuple(p[0], p[1], p[2]); },
            [](uint8_t * p, uint8_t sp1, uint8_t sp2, uint8_t sp3) -> void { p[0] = sp1; p[1] = sp2; p[2] = sp3; },
            [this](int a1, int a2, int b1, int b2, int c) -> auto { return internal::blend(a1, a2, b1, b2, c, g); });
          break;
      }
    }

  public:

    /** \brief class used to encapsulate image drawing
     *
     * When the routine showLayoutSDL needs to draw an image it will call the draw function in this
     * class to do the job. This allows you to do your own image loading and caching and such stuff.
     *
     * Derive from this function and implement the draw function to handle image drawing in your application
     */
    class ImageDrawer_c
    {
      public:
        /** \brief function called to draw an image
         *
         * \param x x-position to draw the image in 1/64 pixels
         * \param y y-position to draw the image in 1/64 pixels
         * \param w width of the image to draw
         * \param h height of the image to draw
         * \param s the SDL-Surface to draw the image on
         * \param url the url of the image to draw
         */
        virtual void draw(int32_t x, int32_t y, uint32_t w, uint32_t h, SDL_Surface * s, const std::string & url) = 0;
    };

    /** \brief display a single layout
     *  \param l layout to draw
     *  \param sx x position on the target surface in 1/64th pixels
     *  \param sy y position on the target surface in 1/64th pixels
     *  \param s target surface
     *  \param sp which kind of sub-pixel positioning do you want?
     *  \param images a pointer to an image drawer class that is used to draw the images, when you give
     *                a nullptr here, no images will be drawn
     *  \param gamma the gamma value of the target surface s in 0.1 increments. Screen surfaces usually have
     *               a gamma of 2.2, if you blit to a intermediate surface use a gamma of one, but then you
     *               should take care of gamma correction when blitting with alpha values.
     *  \note the function uses some lookup tables to quickly calculate the gamma data, that table
     *        is checked at the beginning of this function and recalculated, when necessary, so try
     *        to stick to the same gamma value for long stretches
     */
    void showLayout(const TextLayout_c & l, int sx, int sy, SDL_Surface * s,
                    SubPixelArrangement sp = SUBP_NONE, ImageDrawer_c * images = 0, uint8_t gamma = 22)
    {
      SDL_Rect r;

      g.setGamma(gamma);

      /* render */
      for (auto & i : l.getData())
      {
        switch (i.command)
        {
          case CommandData_c::CMD_GLYPH:
            outputGlyph(sx+i.x, sy+i.y, cache.getGlyph(i.font, i.glyphIndex, sp, i.blurr), sp, g.forward(i.c), s);
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
              outputGlyph(sx+i.x, sy+i.y, cache.getRect(i.w, i.h, sp, i.blurr), sp, g.forward(i.c), s);
            }
            break;

          case CommandData_c::CMD_IMAGE:
            if (images)
              images->draw(i.x+sx, i.y+sy, i.w, i.h, s, i.imageURL);
            break;
        }
      }
    }

    /** \brief trims the font cache down to a maximal number of entries
     *
     * the SDL output module keeps a cache of rendered glyphs to speed up the process of
     * outputting layouts. This cache may get too big on memory. To keep things within limits
     * you can call this function to remove entries.
     * If there are more entries in the cache the ones that were used the longest time ago are removed
     *
     * \param num maximal number of entries, e.g. 0 completely empties the cache
     */
    void trimCache(size_t num)
    {
      cache.trim(num);
    }
};

}

#endif
