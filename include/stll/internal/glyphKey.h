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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __GLYPH_KEY__
#define __GLYPH_KEY__

#include <cstdint>

namespace STLL { namespace internal {

  /** \brief This class can be used as the key in glyph caches, it describes the
   *  exact rendering of the glyph
   */
  class GlyphKey_c
  {
  public:

    GlyphKey_c(std::shared_ptr<FontFace_c> f, glyphIndex_t idx, SubPixelArrangement s, uint16_t b) :
    font((intptr_t)f.get()), glyphIndex(idx), sp(s), blurr(b), w(0), h(0) { }

    GlyphKey_c(int w_, int h_, SubPixelArrangement s, uint16_t b) :
    font(0), glyphIndex(0), sp(s), blurr(b), w(0), h((h_+32)/64)
    {
      switch (sp)
      {
        default:
        case SUBP_NONE:
          w = (w_+32)/64;
          break;

        case SUBP_RGB:
          w = (3*w_+32)/64;
          break;
      }
    }

    intptr_t font;
    glyphIndex_t glyphIndex;
    SubPixelArrangement sp;
    uint16_t blurr;
    uint16_t w, h;

    bool operator==(const GlyphKey_c & a) const
    {
      return          font == a.font
             && glyphIndex == a.glyphIndex
             &&         sp == a.sp
             &&      blurr == a.blurr
             &&          w == a.w
             &&          h == a.h;
    }
  };

} }

namespace std {

  template <>
  class hash<STLL::internal::GlyphKey_c>
  {
  public :
    size_t operator()(const STLL::internal::GlyphKey_c & name ) const
    {
      return (size_t)name.font
           + (size_t)name.glyphIndex
           + (size_t)name.sp
           + (size_t)name.blurr
           + (size_t)name.w
           + (size_t)name.h;
    }
  };

}

#endif
