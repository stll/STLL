#ifndef __LAYOUTER_H__
#define __LAYOUTER_H__

#include "layouterFont.h"

#include <string>
#include <vector>
#include <memory>

// encapsulates a finished layout, the layout is supposed to be unchangeable after creation
class textLayout_c
{
  private:
    uint32_t height; // position where you can add additional text after this ...

  public:

    // information for draw command
    typedef struct
    {
      enum
      {
        CMD_GLYPH,
        CMD_RECT
      } command;

      int32_t x, y; // where to put the glyph
      FT_UInt glyphIndex;
      std::shared_ptr<fontFace_c> font;    // which front to take it from
      uint32_t w, h;  // width height of glyph, used for marking selections, can be 0 for accents and co
      uint8_t r, g, b, a; // rgba color for font foreground

    } commandData;

    // order is the logical order, so when you do copy and paste we copy out of this using the order
    // within the vector
    // TODO find better solution instead of public vector
    std::vector<commandData> data;

    void addCommand(const commandData & d)
    {
      data.push_back(d);
    }

    void addCommandVector(const std::vector<commandData> & d, int dx, int dy)
    {
      for (auto a : d)
      {
        a.x += dx;
        a.y += dy;
        data.push_back(a);
      }
    }

    void append(const textLayout_c & l, int32_t dx, int32_t dy)
    {
      for (auto a : l.data)
      {
        a.x += dx;
        a.y += dy;
        data.push_back(a);
      }

      height = std::max(height, dy+l.height);
    }

    void operator=(textLayout_c && l)
    {
      data.swap(l.data);
      height = l.height;
    }

    void operator=(const textLayout_c & l)
    {
      data = l.data;
      height = l.height;
    }

    ~textLayout_c(void) { }

    textLayout_c(void) : height(0) { }

    textLayout_c(const textLayout_c & src) : height(src.height), data(src.data)  {  }

    textLayout_c(textLayout_c && src)
    {
      swap(data, src.data);
      height = src.height;
    }

    uint32_t getHeight(void) const { return height; }
    void setHeight(uint32_t h) { height = h; }
};

// TODO create a sharing structure, where we only
// define structures for each configuration once
// and link to it
// but for now it is good as it is
typedef struct
{
  uint8_t r, g, b;
  std::shared_ptr<fontFace_c> font;
  std::string lang;
} codepointAttributes;

// this class contains the information about the shape that a paragraph should have
class shape_c
{
  public:
    virtual int32_t getLeft(int32_t top, int32_t bottom) const = 0;
    virtual int32_t getRight(int32_t top, int32_t bottom) const = 0;
};

class rectangleShape_c : public shape_c
{
  private:
    int32_t w;

  public:
    rectangleShape_c(int32_t width) : w(width) { }

    virtual int32_t getLeft(int32_t /*top*/, int32_t /*bottom*/) const { return 0; }
    virtual int32_t getRight(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
};


// the layouting routines

// base layout function that does the layouting stuff for one paragraph
textLayout_c layoutParagraph(const std::u32string & txt32, const std::vector<codepointAttributes> & attr,
                             const shape_c & shape, const std::string & align);

// layout raw text using the font given the given string must be utf-8
textLayout_c layoutRaw(const std::string & txt, const std::shared_ptr<fontFace_c> font, const shape_c & shape, const std::string & language = "en");

#endif
