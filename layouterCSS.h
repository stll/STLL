#ifndef __LAYOUTER_CSS_H__
#define __LAYOUTER_CSS_H__

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

class XhtmlException_c : public std::runtime_error
{
  public:
    explicit XhtmlException_c(const std::string & what_arg) : std::runtime_error(what_arg) {}

};

// this class encapsulates information for HOW to format a text, just like the style sheets
// in html are doing
class textStyleSheet_c
{
  public:

    typedef struct
    {
      std::string selector;
      std::string attribute;
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

    void addRule(const std::string sel, const std::string attr, const std::string val)
    {
      rule r;
      r.selector = sel;
      r.attribute = attr;
      r.value = val;

      rules.push_back(r);
    }

    const std::string & getValue(pugi::xml_node node, const std::string & attribute) const;

    std::string language;   // format: language code: 2 letters, dash, script "de-Latn" TODO remove

  private:
    std::vector<rule> rules;
    std::map<std::string, std::shared_ptr<fontFamily_c> > families;
};

void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b);
double evalSize(const std::string & sz);


#endif
