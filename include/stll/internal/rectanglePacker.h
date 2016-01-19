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
#ifndef __RECTANGLE_PACKER__
#define __RECTANGLE_PACKER__

#include <vector>
#include <array>
#include <experimental/optional>

namespace STLL { namespace internal {

// a class that handles free storage allocation on a two dimensional plane
class RectanglePacker_c
{
  private:
    // area to be filled
    int width_, height_;

    typedef struct
    {
      int x; // x-position start of this skyline section
      int y; // y-position start of this skyline section
      // the end is defined by the start of the next section
      // there is always a last section finalizing the list
    } skyline;

    // the skyline
    std::vector<skyline> skylines;
    std::vector<skyline> skylines_shadow;

    int checkFit(size_t index, int w);

  public:

    RectanglePacker_c(int width, int height);

    uint32_t width(void) const { return width_; }
    uint32_t height(void) const { return height_; }

    // allocate a rectangular area of the given size
    // if none such area is available the optional will be empty
    std::experimental::optional<std::array<uint32_t, 2>> allocate(uint32_t w, uint32_t h);
};

} }

#endif
