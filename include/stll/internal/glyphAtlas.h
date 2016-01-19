
#include "textureAtlas.h"
#include "glyphKey.h"
#include "glyphprepare.h"

#include <vector>

namespace STLL { namespace internal {

class FontAtlasData_c {

  public:
    uint32_t pos_x, pos_y; // top left corner of the image in the texture atlas
    uint32_t rows;   // number of lines, hight of image
    uint32_t width;  // number of pixels per line

    int32_t left;   // where is the left of the image, when the base-point is known
    int32_t top;    // where is the top of the image, when the base-point is known

    FontAtlasData_c(uint32_t posx, uint32_t posy, uint32_t w, uint32_t height, uint32_t l, uint32_t t) :
      pos_x(posx), pos_y(posy), rows(height), width(w), left(l), top(t) {}

    FontAtlasData_c(void) {}
};

// A font atlas for the fonts as used in STLL
//
class GlyphAtlas_c : public TextureAtlas_c<internal::GlyphKey_c, FontAtlasData_c, std::shared_ptr<FontFace_c>, 1>
{
  public:

    const uint16_t blurrmax = 20;

    GlyphAtlas_c(uint32_t width, uint32_t height):
      TextureAtlas_c<internal::GlyphKey_c, FontAtlasData_c, std::shared_ptr<FontFace_c>, 1>(width, height)
    {}

    virtual typename std::unordered_map<internal::GlyphKey_c, FontAtlasData_c>::iterator addElement(const internal::GlyphKey_c & key, const std::shared_ptr<FontFace_c> & f)
    {
      if (f)
      {
        auto g = f->renderGlyph(key.glyphIndex, key.sp);

        std::unordered_map<internal::GlyphKey_c, FontAtlasData_c>::iterator i;

        glyphPrepare(g, key.blurr, key.sp, 1, true,
          [this, key, &i](int w, int h, int l, int t) -> auto {
            i = insert(key, w, h, l, t);
            return std::make_tuple(getData()+i->second.pos_y*width()+i->second.pos_x, width());});

        return i;
      }
      else
      {
        FontFace_c::GlyphSlot_c g(key.w, key.h);

        std::unordered_map<internal::GlyphKey_c, FontAtlasData_c>::iterator i;

        glyphPrepare(g, key.blurr, key.sp, 1, false,
          [this, key, &i](int w, int h, int l, int t) -> auto {
            i = insert(key, w, h, l, t);
            return std::make_tuple(getData()+i->second.pos_y*width()+i->second.pos_x, width());});

        return i;
      }
    }

    FontAtlasData_c getGlyph(std::shared_ptr<FontFace_c> face, glyphIndex_t glyph, SubPixelArrangement sp, uint16_t blurr)
    {
      if (blurr > blurrmax)
      {
        // glyphs with a certain blurr are always without subpixel placement,
        // you'd not recognize the difference
        internal::GlyphKey_c k(face, glyph, SUBP_NONE, blurr);
        return find(k, face).value();
      }
      else
      {
        internal::GlyphKey_c k(face, glyph, sp, blurr);
        return find(k, face).value();
      }
    }

    FontAtlasData_c getRect(int w, int h, SubPixelArrangement, uint16_t blurr)
    {
      // rectangles are always without sub-pixel placement
      internal::GlyphKey_c k(w, h, SUBP_NONE, blurr);
      return find(k, std::shared_ptr<FontFace_c>()).value();
    }
};

} }
