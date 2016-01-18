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

#include "color.h"

#include "dividers.h"
#include "glyphCache.h"

// blitting routines to output the generated glyphs, template code, should be pretty good for
// most purposes

namespace STLL {

/**
 * blending function to blend a glyph pixel on top of an other pixel
 * This function does linear blending incuding a gamma correction
 *
 * \param a1 current value for one channel
 * \param a2 value for that channel to blend over
 * \param b1 alpha value for the previous subpixel
 * \param b2 alpha value for the next subpixel
 * \param c where we are between the 2 subpixels c = 0 -> b1, c = 64 -> b2
 * \param g gamma function to use, g must provide a forward function performing forward gamma correction, inverse
 *          performing inverse correction and scale which returns a scaling value that the gamma corrected values
 *          are scaled with to increase resolution, decrease calculation errors
 */
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

/**
 * Blitting function to paint glyphs
 *
 * This function takes an alpha channel and uses this to alpha blend pixels of a color given onto a surface
 * This alpha channel is assumed to be a "normal" alpha channel, meaning the same resolution in x an y direction
 *
 * \param sx shift value for x, the x position where to output the image, in 1/64 pixels
 * \param sy y position in 1/64 pixels
 * \param img the image to paint
 * \param c color to use for painting
 * \param s pointer to the start of the surface to paint on
 * \param pitch how many bytes per line of pixels
 * \param bbp how many bytes per pixel
 * \param w width in pixels of the surface s
 * \param h height in pixels of the surface s
 * \param pxget function (e.g. lambda) to read out a pixel, the only limit on the concrete values of this function
 *              is that pxget and pxput fit together, so that the 1st return value of this function is the 1st agrument
 *              to pxput. So you don't need to keep the subpixel order in mind here, but you can use the same
 *              functions as you give to outputGlyph_HorizontalRGB
 * \param pxput function (e.g. lambda) that writes a pixel
 * \param blend the function that calculates the blending of the pixel value and the glyph, see function blend for a
 *              description of the arguments
 * \param cx clip rectangle left edge in pixels
 * \param cy clib rectangle upper edge in pixels
 * \param cw width in pixels of the clip rectangle
 * \param ch height in pixels of the clip rectangle
 */
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

/**
 * Blitting function to paint glyphs
 *
 * This function takes an alpha channel and uses this to alpha blend pixels of a color given onto a surface
 * This alpha channel is assumed to be sub-pixel exact that means there are separate alpha values for 3 horizontal
 * subpixels and one value per row, so the alpha values have 3 times the resolution in x direction as in the y direction
 * also the alpha channels are assumed to be at least 2 pixels wider than declared in the img structure. This
 * is to avoid unnecessary checks within this function and so speed up output
 *
 * \param sx shift value for x, the x position where to output the image, in 1/64 pixels
 * \param sy y position in 1/64 pixels
 * \param img the image to paint
 * \param sp1c spc 1-3 define the color to use, they are not rgb, but rather assumed to be the 1st to 3rd subpixel, so
 *             when you got RGB rubpixels put red in sp1c, green in sp2c and blue in sp3c, if you got BGR subpixels put
 *             bluein sp1c and so on
 * \param sp2c see sp1c
 * \param sp3c see sp1c
 * \param alpha the alpha value to use for the glyph to paint
 * \param s pointer to the start of the surface to paint on
 * \param pitch how many bytes per line of pixels
 * \param bbp how many bytes per pixel
 * \param w width in pixels of the surface s
 * \param h height in pixels of the surface s
 * \param pxget function (e.g. lambda) that returns the rgb values of the pixel at a given position, input to the function
 *              is a pointer to the pixel (calculated using pitch and bbp) output is a tuple of 3 uint8_t values representing
 *              the current value of the 1st, 2nd and 3rd subpixel of the pixel. Which are rgb when you have RGB subpixels
 *              or bgr when you have BGR subpixels, and so on
 * \param pxput function (e.g. lambda) that writes a pixel, arguments to the function are the pointer to the pixel and the
 *              3 subpixel values (in subpixel column order)
 * \param blend the function that calculates the blending of the pixel value and the glyph, see function blend for a
 *              description of the arguments
 * \param cx clip rectangle left edge in pixels
 * \param cy clib rectangle upper edge in pixels
 * \param cw width in pixels of the clip rectangle
 * \param ch height in pixels of the clip rectangle
 */
template <class P1, class P2, class B>
void outputGlyph_HorizontalRGB(int sx, int sy, const internal::PaintData_c & img, int sp1c, int sp2c, int sp3c, int alpha,
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
      if (sti > 0) aprev = *(src-1) * alpha;     // when not the first pixel in the glyph line, get previous

      int x = stw;

      uint8_t sp1, sp2, sp3;
      std::tie(sp1, sp2, sp3) = pxget(dst);      // get pixel, there is always at least one to output

      switch (stc)                               // do the remaining sub pixels for the first pixel
      {                                          // all remaining ones are complete
        case 0: a = *src*alpha; sp1 = blend(sp1, sp1c, a, aprev, stb); aprev = a; src++;
        case 1: a = *src*alpha; sp2 = blend(sp2, sp2c, a, aprev, stb); aprev = a; src++;
        case 2: a = *src*alpha; sp3 = blend(sp3, sp3c, a, aprev, stb); aprev = a; src++;
      }

      pxput(dst, sp1, sp2, sp3);
      dst += bbp;
      x--;

      while (x > 0)                              // output the remaining pixels
      {
        std::tie(sp1, sp2, sp3) = pxget(dst);

        a = *src*alpha; sp1 = blend(sp1, sp1c, a, aprev, stb); aprev = a; src++;
        a = *src*alpha; sp2 = blend(sp2, sp2c, a, aprev, stb); aprev = a; src++;
        a = *src*alpha; sp3 = blend(sp3, sp3c, a, aprev, stb); aprev = a; src++;

        pxput(dst, sp1, sp2, sp3);
        dst += bbp;
        x--;
      }
    }
    yp++;
  }
}

}
