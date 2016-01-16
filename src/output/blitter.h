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

#include "cache_single.h"

#include <stll/color.h>

#include "../dividers.h"

// blitting routines to output the generated glyphs, template code, should be pretty good for
// most purposes

namespace STLL
{

// linear blending of 2 values on a surface without alpha channel
template <class G>
int blend(int a1, int a2, int b1, int b2, int c, const G & g)
{
  // check beforehand if we actually have something to blend
  // if the blend factor is zero we return a1
  // amazingly this simple check gives a performance boost
  // of 10-30% depending on the usage
  if (b1 == 0 && (b2== 0 || c == 0)) return a1;

  // the uncorrected blending function would look like this:
  // return a1 + (a2-a1) * b / 255;
  // but that results in wrong values when the target has a gamma
  // corrected output (e.g. display surfaces for sRGB monitors)
  // so we need to correct gamma forward on a1 and inverse on the output
  // the target colour is already corrected
  int b = (int)b1 + ((int)b2-(int)b1)*c/64;


  int d1 = g.forward(a1);
  int d2 = a2*g.scale();

  int out = d1 + (d2-d1)*b/(255*255);

  return g.inverse(out);
}

// glyph rendering without sub-pixel output.
template <class P1, class P2, class B>
void outputGlyph_NONE(int sx, int sy, const internal::PaintData_c & img, Color_c c,
                      uint8_t * s, int pitch, int bbp, int w, int h,
                      const P1 & pxget, const P2 & pxput, const B & blend,
                      int cx = 0, int cy = 0, int cw = std::numeric_limits<int>::max(), int ch = std::numeric_limits<int>::max())
{
  if (cx <= 0) { cw += cx; } else { w -= cx; s += bbp*cx; sx -= 64*cx; }
  if (cy <= 0) { ch += cx; } else { h -= cy; s += pitch*cy; sy -= 64*cy; }
  if (w > cw) { w = cw; }
  if (h > ch) { h = ch; }

  // similar to the glyph output below, see comment there, this one is simpler

  int stx, stb;

  std::tie(stx, stb) = divmod_inf(sx, 64);
  stx += img.left;

  int sty = div_inf(sy+32, 64) - img.top;

  int yp = sty;

  int sti = 0;
  int stw = img.width + 1;

  if (stx < 0)
  {
    sti -= stx;
    stw += stx;
    stx = 0;
  }

  if (stx+stw >= w)
  {
    stw -= (stx+stw-w+1);
  }

  if (stw <= 0) return;
  if (sty >= h || sty+img.rows < 0) return;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < h)
    {
      int xp = stx;

      int a = 0;
      int aprev = 0;

      uint8_t * dst = s + yp*pitch + bbp*xp;
      const uint8_t * src = img.getBuffer() + y*img.pitch + sti;

      if (sti > 0) aprev = *(src-1) * c.a();

      int x = stw;

      while (x > 0)
      {
        a = *src * c.a();

        uint8_t r, g, b;
        std::tie(r, g, b) = pxget(dst);

        r = blend(r, c.r(), a, aprev, stb);
        g = blend(g, c.g(), a, aprev, stb);
        b = blend(b, c.b(), a, aprev, stb);

        pxput(dst, r, g, b);

        aprev = a;
        xp++;
        dst+=bbp;
        src++;
        x--;
      }
    }
    yp++;
  }
}

// glyph rendering with sub-pixel glyph placement
template <class P1, class P2, class B>
void outputGlyph_HorizontalRGB(int sx, int sy, const internal::PaintData_c & img, Color_c c,
                               uint8_t * s, int pitch, int bbp, int w, int h,
                               const P1 & pxget, const P2 & pxput, const B & blend,
                               int cx = 0, int cy = 0, int cw = std::numeric_limits<int>::max(),
                               int ch = std::numeric_limits<int>::max())
{
  if (cx <= 0) { cw += cx; } else { w -= cx; s += bbp*cx; sx -= 64*cx; }
  if (cy <= 0) { ch += cx; } else { h -= cy; s += pitch*cy; sy -= 64*cy; }
  if (w > cw) { w = cw; }
  if (h > ch) { h = ch; }

  int stx = div_inf(sx, 64) + img.left;          // start x pixel position
  int sty = div_inf(sy+32, 64) - img.top;        // start y pixel position
  int stc, stb;

  std::tie(stc, stb) = divmod_inf(3*sx, 64);
  stc = mod_inf(stc, 3);

  int yp = sty;                                  // current y position

  int sti = 0;                                   // image x position start for clipped position
  int stw = img.width/3;                         // width of the clipped image

  if (stx < 0 && stc != 0)                       // when we place image before the first pixel, crop that
  {                                              // first remove possible sub pixels, then the whole pixels
    sti += 3-stc;
    stc = 0;
    stx++;
    stw--;
  }

  if (stx < 0)
  {
    sti -= 3*stx;
    stw += stx;
    stx = 0;
  }

  if (stx+stw >= w)                              // check how much of the image fits into clipping area
  {
    stw -= (stx+stw-w+1);
  }

  if (stw <= 0) return;                          // leave function when there is nothing to output
  if (sty >= h || sty+img.rows < 0) return;

  for (int y = 0; y < img.rows; y++)
  {
    if (yp >= 0 && yp < h)
    {
      int a = 0;                                 // output the line
      int aprev = 0;

      uint8_t * dst = s + yp*pitch + bbp*stx;    // get pointers to glyph and output surface start for line
      const uint8_t * src = img.getBuffer() + y*img.pitch + sti;
      if (sti > 0) aprev = *(src-1) * c.a();     // when not the first pixel in the glyph line, get previous

      int x = stw;

      Uint8 r, g, b;
      std::tie(r, g, b) = pxget(dst);            // get pixel, there is always at least one to output

      switch (stc)                               // do the remaining sub pixels for the first pixel
      {                                          // all remaining ones are complete
        case 0: a = *src*c.a(); r = blend(r, c.g(), a, aprev, stb); aprev = a; src++;
        case 1: a = *src*c.a(); g = blend(g, c.g(), a, aprev, stb); aprev = a; src++;
        case 2: a = *src*c.a(); b = blend(b, c.b(), a, aprev, stb); aprev = a; src++;
      }

      pxput(dst, r, g, b);
      dst += bbp;
      x--;

      while (x > 0)                              // output the remaining pixels
      {
        std::tie(r, g, b) = pxget(dst);

        a = *src*c.a(); r = blend(r, c.r(), a, aprev, stb); aprev = a; src++;
        a = *src*c.a(); g = blend(g, c.g(), a, aprev, stb); aprev = a; src++;
        a = *src*c.a(); b = blend(b, c.b(), a, aprev, stb); aprev = a; src++;

        pxput(dst, r, g, b);
        dst += bbp;
        x--;
      }
    }
    yp++;
  }
}

}
