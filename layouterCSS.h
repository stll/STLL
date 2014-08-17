#ifndef __LAYOUTER_CSS_H__
#define __LAYOUTER_CSS_H__

/** \file
 *  \brief Module containing CSS functionality
 */

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

namespace STLL {

/** \brief exception thrown on XHTML and CSS problems
 */
class XhtmlException_c : public std::runtime_error
{
  public:
    explicit XhtmlException_c(const std::string & what_arg) : std::runtime_error(what_arg) {}

};

/** \brief this class encapsulates information for HOW to format a text, just like the style sheets
 * in html are doing.
 */
class textStyleSheet_c
{
    typedef struct
    {
      std::string selector;
      std::string attribute;
      std::string value;
    } rule;

  public:


    textStyleSheet_c(void) { }

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
    void font(const std::string & family, const fontRessource_c & res,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

    // same as above, but allows you to set the cache
    void font(std::shared_ptr<fontCache_c> fc, const std::string & family, const fontRessource_c & res,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

    std::shared_ptr<fontFamily_c> findFamily(const std::string & family) const {
      return families.find(family)->second;
    }

    void addRule(const std::string sel, const std::string attr, const std::string val);

    // TODO make back into a reference
    const std::string getValue(pugi::xml_node node, const std::string & attribute) const;

  private:
    std::vector<rule> rules;
    std::map<std::string, std::shared_ptr<fontFamily_c> > families;
};

/** \brief evaluate a color string
 *  \param col string with the color
 *  \param r red value of the color
 *  \param g green value of the color
 *  \param b blue value of the color
 */
void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b, uint8_t &a);

/** \brief evaluate size
 *  \param sz the size string from the CSS
 *  \return the resulting size in pixel
 */
double evalSize(const std::string & sz);

}

#endif
