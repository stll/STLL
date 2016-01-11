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

#include <stll/layouterFont.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>

namespace STLL { namespace internal {

// encapsulation for an object to paint it contains the data for the alpha value
// of an object to paint. It is used to store information about single glyphs or
// single rectangles to draw
class PaintData_c
{
  private:

    // copy the buffer and apply a blurr, if needed,
    // when buffer pointer is nullptr, then the whole buffer is assumed to be completely
    // filled with 255 value (used for rectangles)
    void blurrInit(uint8_t * buf_dat, uint16_t buf_pitch, uint16_t buf_rows, uint16_t blurr, SubPixelArrangement sp);

  public:
    int32_t left;  // position of the top left corner relative to the baseposition of the image
    int32_t top;
    int32_t rows;  // hight of image
    int32_t width; // width of image
    int32_t pitch; // number of bytes per line of image, guaranteed to be at least 1 or 2 bigger than width
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t lastUse;

    // create from Freetype glyph data
    PaintData_c(FT_GlyphSlotRec_ * ft, uint16_t blurr, SubPixelArrangement sp);

    // create rectangle data
    PaintData_c(uint16_t width, uint16_t height, uint16_t blurr, SubPixelArrangement sp);

    const uint8_t * getBuffer(void) const { return buffer.get(); }
};

PaintData_c & cache_single_getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp, uint16_t blurr);
PaintData_c & cache_single_getRect(int w, int h, SubPixelArrangement sp, uint16_t blurr);
void cache_single_trim(size_t num);

} }
