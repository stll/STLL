#include "blurr.h"

#include "layouterFont.h"

#include <tuple>

#include <cstring>

namespace STLL {

template <class M>
std::tuple<int, int, int, int, int> glyphPrepare(const FontFace_c::GlyphSlot_c & ft, uint16_t blurr, SubPixelArrangement sp, int frame, bool split, M m)
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

  if (split && (sp != SUBP_NONE))
  {
    // resort image into 3 separate images one for each subpixel
    // pixel column n has to go to n/3 + w/3*(n%3)
    // I found no usable inplace algorithm, so copy line by line
    // into a new array and then copy the array back
    std::vector<uint8_t> l(pitch);

    for (int j = 0; j < rows; j++)
    {
      for (int i = 0; i < pitch; i++)
        l[(i/3) + (pitch/3)*(i%3)] = outbuf_dat[j*outbuf_pitch+i];

      memcpy(outbuf_dat+j*outbuf_pitch, l.data(), pitch);
    }
  }

  return std::make_tuple(left, top, width, pitch, rows);
}

}
