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
#include <stll/blurr.h>

#include <array>
#include <cmath>
#include <memory>

namespace STLL { namespace internal {

#define BLURR_N 3

static std::array<uint8_t, BLURR_N> boxesForGauss(double rad)  // standard deviation, number of boxes
{
  double wideal = sqrt((12.0*rad*rad/BLURR_N)+1);  // Ideal averaging filter width
  int wl = floor(wideal);
  if (wl % 2 == 0) wl--;
  int wu = wl+2;

  double mideal = (12.0*rad*rad - BLURR_N*wl*wl - 4*BLURR_N*wl - 3*BLURR_N)/(-4*wl - 4);
  int m = round(mideal);

  std::array<uint8_t, BLURR_N> a;
  for(int i = 0; i < BLURR_N; i++) a[i] = (i<m) ? wl : wu;
  return a;
}

static void boxBlurH_4 (uint8_t * s, int spitch, uint8_t *d, int dpitch, int w, int h, int r)
{
  double iarr = 1.0 / (r+r+1);
  for(int i = 0; i < h; i++)
  {
    int ti = i*dpitch;
    int li = i*spitch;
    int ri = li+r;
    int fv = s[li];
    int lv = s[li+w-1];
    int val = (r+1)*fv;
    for (int j=0;   j < r; j++) val += s[li+j];
    for (int j=0;   j<= r; j++) { val += s[ri++] - fv     ; d[ti++] = round(val*iarr); }
    for (int j=r+1; j<w-r; j++) { val += s[ri++] - s[li++]; d[ti++] = round(val*iarr); }
    for (int j=w-r; j<w  ; j++) { val += lv      - s[li++]; d[ti++] = round(val*iarr); }
  }
}

static void boxBlurT_4 (uint8_t * s, int spitch, uint8_t * d, int dpitch, int w, int h, int r)
{
  double iarr = 1.0 / (r+r+1);
  for(int i=0; i < w; i++)
  {
    int ti = i;
    int li = ti;
    int ri = ti+r*spitch;
    int fv = s[ti];
    int lv = s[ti+spitch*(h-1)];
    int val = (r+1)*fv;
    for (int j=0;   j < r; j++) val += s[ti+j*spitch];
    for (int j=0  ; j<= r; j++) { val += s[ri] - fv   ; d[ti] = round(val*iarr); ri+=spitch; ti+=dpitch; }
    for (int j=r+1; j<h-r; j++) { val += s[ri] - s[li]; d[ti] = round(val*iarr); li+=spitch; ri+=spitch; ti+=dpitch; }
    for (int j=h-r; j<h  ; j++) { val += lv    - s[li]; d[ti] = round(val*iarr); li+=spitch; ti+=dpitch; }
  }
}

void gaussBlur (uint8_t * s, int pitch, int w, int h, double r, int sx, int sy)
{
  auto a = boxesForGauss(r/2);
  auto d = std::make_unique<uint8_t[]>(w*h);
  boxBlurT_4(s, pitch, d.get(), w, w, h, sy*(a[0]-1)/2);
  boxBlurH_4(d.get(), w, s, pitch, w, h, sx*(a[0]-1)/2);
  boxBlurT_4(s, pitch, d.get(), w, w, h, sy*(a[1]-1)/2);
  boxBlurH_4(d.get(), w, s, pitch, w, h, sx*(a[1]-1)/2);
  boxBlurT_4(s, pitch, d.get(), w, w, h, sy*(a[2]-1)/2);
  boxBlurH_4(d.get(), w, s, pitch, w, h, sx*(a[2]-1)/2);
}

} }