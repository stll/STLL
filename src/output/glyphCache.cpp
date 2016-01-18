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

#include <stll/glyphCache.h>

#include <stll/layouter.h>
#include <stll/color.h>
#include <stll/blurr.h>

#include <stll/glyphKey.h>
#include <stll/glyphprepare.h>


#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>

// TODO properly handle it, when FreeType returns an bitmap format that is not supported

namespace STLL { namespace internal {

// create from glyph data
PaintData_c::PaintData_c(const FontFace_c::GlyphSlot_c & ft, uint16_t blurr, SubPixelArrangement sp)
{
  std::tie(left, top, width, pitch, rows) = glyphPrepare(ft, blurr, sp, 0,
    [this](int w, int h, int, int) -> auto {
      buffer = std::make_unique<uint8_t[]>(w*h);
      return std::make_tuple(buffer.get(), w);});
}

// create rectangle data
PaintData_c::PaintData_c(uint16_t _pitch, uint16_t _rows, uint16_t blurr, SubPixelArrangement sp)
{
  FontFace_c::GlyphSlot_c ft(_pitch, _rows);

  std::tie(left, top, width, pitch, rows) = glyphPrepare(ft, blurr, sp, 0,
    [this](int w, int h, int, int) -> auto {
      buffer = std::make_unique<uint8_t[]>(w*h);
      return std::make_tuple(buffer.get(), w);});
}

// get the glyph from the cache, or render new using FreeType
PaintData_c & GlyphCache_c::getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp, uint16_t blurr)
{
  GlyphKey_c k(face, glyph, sp, blurr);

  auto i = glyphCache.find(k);

  if (i == glyphCache.end())
  {
    auto g = face->renderGlyph(glyph, sp);


    i = glyphCache.insert(std::make_pair(k, std::move(PaintData_c(g, blurr, sp)))).first;
  }

  i->second.lastUse = useCounter;
  useCounter++;

  return i->second;
}

PaintData_c & GlyphCache_c::getRect(int w, int h, SubPixelArrangement sp, uint16_t blurr)
{
  GlyphKey_c k(w, h, sp, blurr);

  auto i = glyphCache.find(k);

  if (i == glyphCache.end())
  {
    i = glyphCache.insert(std::make_pair(k, std::move(PaintData_c(k.w, k.h, k.blurr, k.sp)))).first;
  }

  return i->second;
}

void GlyphCache_c::trim(size_t num)
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
}

} }
