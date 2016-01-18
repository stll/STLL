
#include <stll/rectanglePacker.h>

namespace STLL { namespace internal {

// try to place the rectangle at the start of section with the given
// index, we return the resulting y-position
int RectanglePacker_c::checkFit(size_t index, int w)
{
  int xend = skylines[index].x+w;
  int y = skylines[index].y;
  index++;

  while (index < skylines.size() && skylines[index].x < xend)
  {
    y = std::max(y, skylines[index].y);
    index++;
  }

  return y;
}

RectanglePacker_c::RectanglePacker_c(int width, int height) : width_(width), height_(height)
{
  skylines.push_back(skyline {1, 1});
  skylines.push_back(skyline {width-1, height});
}

std::experimental::optional<std::array<uint32_t, 2>> RectanglePacker_c::allocate(uint32_t w, uint32_t h)
{
  size_t bestI = 0;
  uint16_t bestY = checkFit(bestI, w);

  for (size_t i = 1; skylines[i].x+(int)w+1 < width_; i++)
  {
    uint16_t y = checkFit(i, w);
    if (y < bestY)
    {
      bestY = y;
      bestI = i;
    }
  }

  // get the resulting x position
  int bestX = skylines[bestI].x;

  // calculate the next y position for that section
  int nextY = bestY + h;

  if (nextY+1 >= height_)
  {
    // doesn't fit
    return std::experimental::optional<std::array<uint32_t, 2>>();
  }

  // update skyline, we use a 2nd vector for this
  // first copy stuff over to this vector, then swap the data
  skylines_shadow.clear();
  skylines_shadow.reserve(skylines.size()+2);

  // 1st copy everything up to the new skyline segment
  size_t i = 0;
  while (i < bestI)
  {
    skylines_shadow.push_back(skylines[i]);
    i++;
  }

  // if the new segment has a different height than
  // the one before, add a new segment
  if (i == 0 || nextY != skylines[i-1].y)
  {
    skylines_shadow.push_back(skyline{skylines[i].x, nextY});
  }

  uint16_t xend = skylines[bestI].x+w;

  // skip all segments until we are at the segment that starts behind us
  // (at least the last one will do so)
  while (skylines[i].x < xend)
    i++;

  if (skylines[i].x > xend && skylines[i-1].y != nextY)
  {
    skylines_shadow.push_back(skyline{xend, skylines[i-1].y});
  }

  // copy remainder
  while (i < skylines.size())
  {
    skylines_shadow.push_back(skylines[i]);
    i++;
  }

  swap(skylines, skylines_shadow);

  return std::array<uint32_t, 2>{(uint32_t)bestX, (uint32_t)bestY};
}

} }
