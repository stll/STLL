#ifndef __LAYOUTER_H__
#define __LAYOUTER_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "layouterFont.h"

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

    ~textLayout_c(void) { }

    textLayout_c(void) { }

    textLayout_c(const textLayout_c & src) : data(src.data) {  }

    textLayout_c(textLayout_c && src)
    {
      swap(data, src.data);
    }

};


// this class encapsulates information for HOW to format a text, just like the style sheets
// in html are doing
class textStyleSheet_c
{
  public:

    typedef struct
    {
      std::string selector;
      std::string property;
      std::string value;
    } rule;

    textStyleSheet_c(void) : language("en-Latn") {}

    // add a font to a family
    // family is the name of the font familiy to add to
    // file the ttf (or other freetype) font file
    // the remaining parameters correspond to the style settings
    // that a font can have in html
    //
    // !ATTENTION!, this version will automatically set a fontCache_c class
    // for each family, this will either be the same class as an already
    // existing family, or a new cache if there is no font family
    // already in this style. If you have multiple styles and want to
    // share caches, you will need to at least once call the
    // other version of this function below
    void font(const std::string & family, const std::string & file,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

    // same as above, but allows you to set the cache
    void font(std::shared_ptr<fontCache_c> fc, const std::string & family, const std::string & file,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

    std::shared_ptr<fontFamily_c> findFamily(const std::string & family) const { return families.find(family)->second; }

    void addRule(const std::string sel, const std::string prop, const std::string val)
    {
      rule r;
      r.selector = sel;
      r.property = prop;
      r.value = val;

      rules.push_back(r);
    }

    const std::string & getValue(const pugi::xml_node & node, const std::string & attribute) const;

    std::string language;   // format: language code: 2 letters, dash, script "de-Latn" TODO remove

  private:
    std::vector<rule> rules;
    std::map<std::string, std::shared_ptr<fontFamily_c> > families;
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

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return 0; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return w; }
};

textLayout_c layout(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape);

#endif
