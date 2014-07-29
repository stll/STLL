#ifndef __LAYOUTER_H__
#define __LAYOUTER_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "layouterFont.h"
#include "layouterCSS.h"

#include <boost/utility.hpp>

#include <pugixml.hpp>

#include <string>
#include <memory>
#include <assert.h>
#include <vector>
#include <map>

// encapsulates a finished layout, the layout is supposed to be unchangeable after creation
class textLayout_c
{
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
      data.insert(data.end(), l.data.begin(), l.data.end());
    }

    void operator=(textLayout_c && l)
    {
      data.swap(l.data);
    }

    void operator=(const textLayout_c & l)
    {
      data = l.data;
    }

    ~textLayout_c(void) { }

    textLayout_c(void) { }

    textLayout_c(const textLayout_c & src) : data(src.data) {  }

    textLayout_c(textLayout_c && src)
    {
      swap(data, src.data);
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

// the layouting routines

// layout the given XHTML code
textLayout_c layoutXHTML(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape);

// layout the given pugi-XML nodes, they must be a parsed XHTML document
textLayout_c layoutXML(const pugi::xml_document & txt, const textStyleSheet_c & rules, const shape_c & shape);

// layout raw text using the font given the given string must be utf-8
textLayout_c layoutRaw(const std::string & txt, const std::shared_ptr<fontFace_c> font, const shape_c & shape, const std::string & language = "en");

#endif
