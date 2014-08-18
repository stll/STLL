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

/** \brief this class encapsulates information for how to format a text, just like the
 * style sheets in html are doing.
 *
 * This class contains a list of normal CSS rules as well as a list of font families
 * to use for outputting text.
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

    /** \brief create an empty style sheet only with default rules */

    /* TODO simplify: only one cache per stylesheet, and make it possible to specify here */
    textStyleSheet_c(void) { }

    /** \brief Add a font to a family.
     *
     * This function will add a new font to a family within this stylesheet.
     * If the given family doesn't exist, it will be created
     *
     * The class will use the same font cache and thus the same instance of the
     * freetype library for all the fonts. So you can only use it from one thread.
     *
     * \param family The name of the font family that gets a new member
     * \param res The resource for the new family member
     * \param style See fontFamily_c::getFont()
     * \param variant See fontFamily_c::getFont()
     * \param weight See fontFamily_c::getFont()
     * \param stretch See fontFamily_c::getFont()
     */
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

    /** \brief Get a font family from the CSS
     *
     * \param family The family you want to get
     * \returns The family of nullptr
     */
    std::shared_ptr<fontFamily_c> findFamily(const std::string & family) const {
      return families.find(family)->second;
    }

    /** \brief add a rule to the stylesheet
     *
     * \param sel The CSS selector (see \ref css_sec for supported selectors)
     * \param attr The attribute this rule applies to (see \ref css_sec for supported attributes)
     * \param val The value for the attribute
     */
    void addRule(const std::string sel, const std::string attr, const std::string val);


    /** \brief get the value for an attribute for a given xml-node
     *
     * \param node The xml node that the attribute value is requested for
     * \param attribute The attribute the value is requested for
     *
     * \return The value of the attribute
     */
    // TODO make back into a reference
    const std::string getValue(pugi::xml_node node, const std::string & attribute) const;

  private:
    std::vector<rule> rules;
    std::map<std::string, std::shared_ptr<fontFamily_c> > families;
};

}

#endif
