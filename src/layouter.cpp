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

#include <stll/layouter.h>

namespace STLL {

TextLayout_c::TextLayout_c(TextLayout_c&& src) :
height(src.height), left(src.left), right(src.right), firstBaseline(src.firstBaseline),
data(std::move(src.data)), links(std::move(src.links)) { }

TextLayout_c::TextLayout_c(const TextLayout_c& src):
height(src.height), left(src.left), right(src.right), firstBaseline(src.firstBaseline),
data(src.data), links(src.links) { }

TextLayout_c::TextLayout_c(void): height(0), left(0), right(0), firstBaseline(0) { }

void TextLayout_c::append(const TextLayout_c & l, int dx, int dy)
{
  if (data.empty())
    firstBaseline = l.firstBaseline + dy;

  for (auto a : l.data)
  {
    a.x += dx;
    a.y += dy;
    data.push_back(a);
  }

  for (auto a : l.links)
  {
    links.push_back(LinkInformation_c(a));
    for (auto b : links.back().areas)
    {
      b.x += dx;
      b.y += dy;
    }
  }

  height = std::max(height, l.height);
  left = std::min(left, l.left);
  right = std::max(right, l.right);
}

void TextLayout_c::shift(int32_t dx, int32_t dy)
{
  for (auto & a : data)
  {
    a.x += dx;
    a.y += dy;
  }

  for (auto & l : links)
    for (auto & a: l.areas)
    {
      a.x += dx;
      a.y += dy;
    }
}

}
