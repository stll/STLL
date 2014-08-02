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

    void append(const textLayout_c & l)
    {
      for (auto a : l.data)
        data.push_back(a);

      height = std::max(height, l.height);
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

// this structure contains all attributes that a single glyph can get assigned
typedef struct
{
  // color of the letter
  uint8_t r, g, b;
  // font of the letter
  std::shared_ptr<fontFace_c> font;
  // of which languages it is
  std::string lang;

  // TODO things like underline and strike-through are missing
  // TODO shadows
} codepointAttributes;


class attributeIndex_c
{
  private:
    std::vector<codepointAttributes> attr;

  public:
    attributeIndex_c(void) { }
    attributeIndex_c(const codepointAttributes & a) { attr.push_back(a); }

    void set(size_t i, codepointAttributes a)
    {
      if (i >= attr.size())
        attr.resize(i+1);
      attr[i] = a;
    }

   void set(size_t start, size_t end, codepointAttributes a)
   {
     if (end > attr.size())
       attr.resize(end);

     for (size_t i = start; i < end; i++)
       attr[i] = a;
   }

    const codepointAttributes & get(size_t i) const
    {
      if (i < attr.size())
        return attr[i];
      else
        return attr[0];
    }
};

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

typedef struct {
  enum {
    ALG_LEFT,
    ALG_RIGHT,
    ALG_CENTER,
    ALG_JUSTIFY_LEFT,
    ALG_JUSTIFY_RIGHT
  } align;

  int32_t indent;

} layoutProperties;



// the layouting routines

// base layout function that does the layouting stuff for one paragraph
textLayout_c layoutParagraph(const std::u32string & txt32, const attributeIndex_c & attr,
                             const shape_c & shape, const layoutProperties & prop, int32_t ystart);

#endif
