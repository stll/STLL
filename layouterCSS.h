/*
 * STLL Simple Text Layouting Library
 *
 * STLL is the legal property of its developers, whose
 * names are listed in the COPYRIGHT file, which is included
 * within the source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __LAYOUTER_CSS_H__
#define __LAYOUTER_CSS_H__

/** \file
 *  \brief Module containing CSS functionality
 */

#include "layouterFont.h"

#include <pugixml.hpp>

#include <string>
#include <memory>
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

    /** \brief create an empty style sheet only with default rules
     *
     * If you do not specify a default cache, the style sheet will create its
     * own cache with its own library instance
     *
     * This cache will be used for all fonts of the style sheet.
     */
    textStyleSheet_c(std::shared_ptr<fontCache_c> c = 0);

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
    void font(const std::string & family, const fontResource_c & res,
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
     * The rule will be checked as much as possible against syntactic and semantic
     * Problems, when a problem is detected (e.g. invalid values) the XhtmlException_c
     * is thrown.
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
    const std::string & getValue(pugi::xml_node node, const std::string & attribute) const;

    /** \brief set the rounding factor. See layoutProperties.round for the details.
     *
     * This tells the HTML layouter, which value to tell the paragraph layouter
     */
    void setRound(uint32_t val) { round = val; }

    /** \brief get the rounding factor. See layoutProperties.round for the details.
     */
    uint32_t getRound(void) const { return round; }

  private:
    std::vector<rule> rules;
    std::map<std::string, std::shared_ptr<fontFamily_c> > families;
    std::shared_ptr<fontCache_c> cache;
    uint32_t round;
};

}

#endif
