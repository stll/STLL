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
#ifndef STLL_GLYPH_PREPARE_H
#define STLL_GLYPH_PREPARE_H

#include "blurr.h"

#include "../layouterFont.h"

#include <tuple>

#include <cstring>

namespace STLL { namespace internal {

template <class M>
std::tuple<int, int, int, int, int> glyphPrepare(const FontFace_c::GlyphSlot_c & ft, uint16_t blurr, SubPixelArrangement sp, int frame, M m)
{
  // create a buffer that is big enough to hold the complete blurred image
  // and has an additional number of columns on the right so that no additional
  // checks are needed for drawing for the last column
  // so the pitch of the buffered image is always 1 or 2 columns wider than the image width (depending on suppixel)
  int blurrdist = internal::gaussBlurrDist(blurr/64.0);
  int blurrh = 1;
  int blurrw = 1;
  int addc = 1;
  int ts = blurrdist;
  int ls = blurrdist;

  int left = ft.left - ls;
  int top = ft.top + ts;

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

  int width = ft.w + 2*ls + frame;
  int pitch = ft.w + 2*ls + frame + addc;
  int rows  = ft.h + 2*ts + frame;

  uint8_t * outbuf_dat;
  uint32_t outbuf_pitch;

  std::tie(outbuf_dat, outbuf_pitch) = m(pitch, rows, left, top);

  if (outbuf_dat)
  {
    // copy image into the centre of the enlarged buffer
    for (int i = 0; i < ft.h; i++)
    {
      if (ft.data)
        memcpy(outbuf_dat+(i+ts)*outbuf_pitch+ls, ft.data+i*(ft.pitch), ft.w);
      else
        memset(outbuf_dat+(i+ts)*outbuf_pitch+ls, 255, ft.w);
    }

    if (blurr > 0)
    {
      internal::gaussBlur(outbuf_dat, outbuf_pitch, pitch, rows, blurr/64.0, blurrw, blurrh);
    }

    return std::make_tuple(left, top, width, pitch, rows);
  }
  else
  {
    return std::make_tuple(0, 0, 0, 0, 0);
  }
}

} }

#endif
