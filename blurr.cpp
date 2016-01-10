
#include "blurr.h"

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

static void boxBlurH_4 (uint8_t * s, uint8_t *d, int w, int h, int r)
{
  double iarr = 1.0 / (r+r+1);
  for(int i = 0; i < h; i++)
  {
    int ti = i*w;
    int li = ti;
    int ri = ti+r;
    int fv = s[ti];
    int lv = s[ti+w-1];
    int val = (r+1)*fv;
    for (int j=0;   j < r; j++) val += s[ti+j];
    for (int j=0;   j<= r; j++) { val += s[ri++] - fv     ; d[ti++] = round(val*iarr); }
    for (int j=r+1; j<w-r; j++) { val += s[ri++] - s[li++]; d[ti++] = round(val*iarr); }
    for (int j=w-r; j<w  ; j++) { val += lv      - s[li++]; d[ti++] = round(val*iarr); }
  }
}

static void boxBlurT_4 (uint8_t * s, uint8_t * d, int w, int h, int r)
{
  double iarr = 1.0 / (r+r+1);
  for(int i=0; i < w; i++)
  {
    int ti = i;
    int li = ti;
    int ri = ti+r*w;
    int fv = s[ti];
    int lv = s[ti+w*(h-1)];
    int val = (r+1)*fv;
    for (int j=0;   j < r; j++) val += s[ti+j*w];
    for (int j=0  ; j<= r; j++) { val += s[ri] - fv   ; d[ti] = round(val*iarr); ri+=w; ti+=w; }
    for (int j=r+1; j<h-r; j++) { val += s[ri] - s[li]; d[ti] = round(val*iarr); li+=w; ri+=w; ti+=w; }
    for (int j=h-r; j<h  ; j++) { val += lv    - s[li]; d[ti] = round(val*iarr); li+=w; ti+=w; }
  }
}

void gaussBlur (uint8_t * s, int w, int h, double r, int sx, int sy)
{
  auto a = boxesForGauss(r/2);
  auto d = std::make_unique<uint8_t[]>(w*h);
  boxBlurT_4(s, d.get(), w, h, sy*(a[0]-1)/2);
  boxBlurH_4(d.get(), s, w, h, sx*(a[0]-1)/2);
  boxBlurT_4(s, d.get(), w, h, sy*(a[1]-1)/2);
  boxBlurH_4(d.get(), s, w, h, sx*(a[1]-1)/2);
  boxBlurT_4(s, d.get(), w, h, sy*(a[2]-1)/2);
  boxBlurH_4(d.get(), s, w, h, sx*(a[2]-1)/2);
}

} }