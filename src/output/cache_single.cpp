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

#include <stll/layouter.h>
#include <stll/color.h>
#include "blurr.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>


// TODO properly handle it, when FreeType returns an bitmap format that is not supported


namespace STLL { namespace internal {


/** \brief This class can be used as the key in glyph caches, it describes the
 *  exact rendering of the glyph
 */
class GlyphKey_c
{
  public:

    GlyphKey_c(std::shared_ptr<FontFace_c> f, glyphIndex_t idx, SubPixelArrangement s, uint16_t b) :
      font((intptr_t)f.get()), glyphIndex(idx), sp(s), blurr(b) { }

    intptr_t font;
    glyphIndex_t glyphIndex;
    SubPixelArrangement sp;
    uint16_t blurr;

    bool operator==(const GlyphKey_c & a) const
    {
      return font == a.font && glyphIndex == a.glyphIndex && sp == a.sp && blurr == a.blurr;
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
    return (size_t)name.font ^ (size_t)name.glyphIndex ^ (size_t)name.sp ^ name.blurr;
  }
};

}



namespace STLL { namespace internal {

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

} }


namespace std {

template <>
class hash<STLL::internal::RectKey_c>
{
  public :
  size_t operator()(const STLL::internal::RectKey_c & name ) const
  {
    return (size_t)name.w * (size_t)name.h * (size_t)name.blurr * (size_t)(name.sp);
  }
};

}



namespace STLL { namespace internal {

// our glyph cache with all the rendered glyphs
static std::unordered_map<GlyphKey_c, PaintData_c> glyphCache;
static std::unordered_map<RectKey_c, PaintData_c> rectCache;

// each time we access a glyph from the cache we increase this number
// and write the value into the lastUse field of the rendered glyph
// that is how we can find out glyphs that were not used the longest time
static uint32_t useCounter = 0;


// TODO properly handle it, when FreeType returns an bitmap format that is not supported

// copy the buffer and apply a blurr, if needed, when buffer pointer
// is nullptr, then the whole buffer is assumed to be completely filled with 255 value (used
// for rectangles)
void PaintData_c::blurrInit(uint8_t * buf_dat, uint16_t buf_pitch, uint16_t buf_rows, uint16_t blurr, SubPixelArrangement sp)
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
    internal::gaussBlur(buffer.get(), pitch, rows, blurr/64.0, blurrw, blurrh);
  }
}


// create from glyph data
PaintData_c::PaintData_c(FT_GlyphSlotRec_ * ft, uint16_t blurr, SubPixelArrangement sp) :
left(ft->bitmap_left), top(ft->bitmap_top), rows(ft->bitmap.rows),
width(ft->bitmap.width), pitch(ft->bitmap.pitch)
{
  blurrInit(ft->bitmap.buffer, ft->bitmap.pitch, ft->bitmap.rows, blurr, sp);
}

// create rectangle data
PaintData_c::PaintData_c(uint16_t _pitch, uint16_t _rows, uint16_t blurr, SubPixelArrangement sp) :
left(0), top(0), rows(_rows), width(_pitch), pitch(_pitch)
{
  blurrInit(0, _pitch, _rows, blurr, sp);
}


// get the glyph from the cache, or render new using FreeType
PaintData_c & cache_single_getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp, uint16_t blurr)
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

PaintData_c & cache_single_getRect(int w, int h, SubPixelArrangement sp, uint16_t blurr)
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

void cache_single_trim(size_t num)
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

} }
