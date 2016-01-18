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
