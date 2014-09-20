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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE STLL Tests
#include <boost/test/unit_test.hpp>

#include "layouterCSS.h"
#include "layouterXHTML.h"
#include "layouterFont.h"
#include "layouterXMLSaveLoad.h"

#include <pugixml.hpp>

#include <string>

static bool compare(const STLL::textLayout_c & l, const pugi::xml_node & doc, std::shared_ptr<STLL::fontCache_c> c)
{
  if (l.getHeight() != std::stoi(doc.attribute("height").value())) return false;
  if (l.getLeft() != std::stoi(doc.attribute("left").value())) return false;
  if (l.getRight() != std::stoi(doc.attribute("right").value())) return false;

  // get the fonts from the file
  auto fonts = doc.child("fonts");
  std::vector<std::pair<std::string, uint32_t>> found;

  for (const auto a : fonts.children())
      found.push_back(std::make_pair(a.attribute("file").value(), std::stoul(a.attribute("size").value())));

  auto commands = doc.child("commands");

  size_t i = 0;

  for (const auto a : commands.children())
  {
    if (i >= l.data.size()) return false;

    if (a.name() == std::string("glyph"))
    {
      if (l.data[i].command != STLL::textLayout_c::commandData::CMD_GLYPH) return false;
      if (l.data[i].x != std::stoi(a.attribute("x").value())) return false;
      if (l.data[i].y != std::stoi(a.attribute("y").value())) return false;
      if (l.data[i].glyphIndex != std::stoi(a.attribute("glyphIndex").value())) return false;

      int f = std::stoi(a.attribute("font").value());
      if (c->getFontResource(l.data[i].font).getDescription() != found[f].first) return false;
      if (c->getFontSize(l.data[i].font) != found[f].second) return false;

      if (l.data[i].c.r() != std::stoi(a.attribute("r").value())) return false;
      if (l.data[i].c.g() != std::stoi(a.attribute("g").value())) return false;
      if (l.data[i].c.b() != std::stoi(a.attribute("b").value())) return false;
      if (l.data[i].c.a() != std::stoi(a.attribute("a").value())) return false;
    }
    else if (a.name() == std::string("rect"))
    {
    }
    else if (a.name() == std::string("image"))
    {
    }
    else
    {
      return false;
    }

    i++;
  }

  return i == l.data.size();
}


static boost::test_tools::predicate_result layouts_identical(const  STLL::textLayout_c & l,
                                                             const std::string & file,
                                                             std::shared_ptr<STLL::fontCache_c> c)
{
  pugi::xml_document doc;

  auto res = doc.load_file(file.c_str());

  if (!res || !compare(l, doc.child("layout"), c))
  {
    doc.reset();
    saveLayoutToXML(l, doc, c);
    doc.save_file((file+"new").c_str());

    return false;
  }

  return true;
}

BOOST_AUTO_TEST_CASE( Stylesheet_Resource_Tests )
{
  STLL::textStyleSheet_c s;

  //************************************
  // check invalid selectors
  //************************************

  // invalid tag with normal tag selector
  BOOST_CHECK_THROW(s.addRule("blubb", "color", "#000000"), STLL::XhtmlException_c);
  // invalid tag with attribute selector
  BOOST_CHECK_THROW(s.addRule("blubb[lang|=en]", "color", "#000000"), STLL::XhtmlException_c);
  // invalid attribute with attribute selector
  BOOST_CHECK_THROW(s.addRule("p[slang|=en]", "color", "#000000"), STLL::XhtmlException_c);
  // invalid operation with attribute selector
  BOOST_CHECK_THROW(s.addRule("p[lang~=en]", "color", "#000000"), STLL::XhtmlException_c);
  // missing closing square bracket
  BOOST_CHECK_THROW(s.addRule("p[lang|=en", "color", "#000000"), STLL::XhtmlException_c);
  // missing operator square bracket
  BOOST_CHECK_THROW(s.addRule("p[lang=en]", "color", "#000000"), STLL::XhtmlException_c);

  //************************************
  // check invalid attributes
  //************************************

  // check that unknown attributes are detected
  BOOST_CHECK_THROW(s.addRule("p", "colour", "#000000"), STLL::XhtmlException_c);

  //************************************
  // check invalid values
  //************************************

  // invalid size:, x missing
  BOOST_CHECK_THROW(s.addRule("p", "font-size", "102p"), STLL::XhtmlException_c);
  // invalid size: invalid number
  BOOST_CHECK_THROW(s.addRule("p", "font-size", "10A2px"), STLL::XhtmlException_c);
  // invalid size: invalid number
  BOOST_CHECK_THROW(s.addRule("p", "font-size", "1A02%"), STLL::XhtmlException_c);
  // invalid size: no unit
  BOOST_CHECK_THROW(s.addRule("p", "font-size", "10"), STLL::XhtmlException_c);
  // invalid size: negative size
  BOOST_CHECK_THROW(s.addRule("p", "font-size", "-10px"), STLL::XhtmlException_c);

  // invalid shadow list: unit wrong 1st number
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12p 12px #2034ff , 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: unit wrong 2nd number
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12x #2034ff , 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: colour not enough digits
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px -12px #204ff , 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: comma missing
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff  12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: trailing comma
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff , -12px 12px #121212,"), STLL::XhtmlException_c);
  // invalid shadow list: colour missing at the end
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff , 12px -12px"), STLL::XhtmlException_c);
  // invalid shadow list: not enough digits for colour
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034f, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: invalid digit in colour
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034fg, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: y in size unit
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12py 12px #2034ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: too many digits in colour letter
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034fff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: too many digits in colour number
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff0, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: hash in wrong position
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px #12px #2034ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: additional minus in offset
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-1-2px 12px #2034ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: additional minus in colour
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #20-3ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: digit before the coloud #
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px 1#2034ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: letter in offset number
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff, 12px 12Apx #121212"), STLL::XhtmlException_c);
  // invalid shadow list: unit improperly spelled
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12pxp 12px #2034ff, 12px 12px #121212"), STLL::XhtmlException_c);
  // invalid shadow list: not enough digits in last colour
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px #2034ff, 12px 12px #12112"), STLL::XhtmlException_c);
  // invalid shadow list: missing space 1
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px12px #2034ff, 12px 12px #1211F2"), STLL::XhtmlException_c);
  // invalid shadow list: missing space 2
  BOOST_CHECK_THROW(s.addRule("p", "text-shadow", "-12px 12px#2034ff, 12px 12px #1211F2"), STLL::XhtmlException_c);

  // invalid colour: hash missing
  BOOST_CHECK_THROW(s.addRule("p", "color", "000000"), STLL::XhtmlException_c);
  // invalid colour: not enough digits
  BOOST_CHECK_THROW(s.addRule("p", "color", "#00000"), STLL::XhtmlException_c);
  // invalid colour: too many digits
  BOOST_CHECK_THROW(s.addRule("p", "color", "#00ABC00"), STLL::XhtmlException_c);
  // invalid colour: not allowed symbol in colour value
  BOOST_CHECK_THROW(s.addRule("p", "color", "#ABCFG0"), STLL::XhtmlException_c);

  // wrong value for a string attribute
  BOOST_CHECK_THROW(s.addRule("p", "direction", "lr"), STLL::XhtmlException_c);
}

BOOST_AUTO_TEST_CASE( Faulty_XHTML_Code )
{
  STLL::textStyleSheet_c s;
  STLL::rectangleShape_c r(200*64);

  s.addRule("p", "font-size", "16px");
  s.font("sans", STLL::fontResource_c("tests/FreeSans.ttf"));

  // not properly closing tag
  BOOST_CHECK_THROW(layoutXHTML("<html><body><p>Text</p></body></htm>", s, r), STLL::XhtmlException_c);
  // html is not the top most tag
  BOOST_CHECK_THROW(layoutXHTML("<htm><body><p>Text</p></body></htm>", s, r), STLL::XhtmlException_c);
  // no body tag
  BOOST_CHECK_THROW(layoutXHTML("<html><p>Text</p></html>", s, r), STLL::XhtmlException_c);
  // something else beside head and body in html tag
  BOOST_CHECK_THROW(layoutXHTML("<html><hed></hed><body><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // text outside of the p tag
  BOOST_CHECK_THROW(layoutXHTML("<html><head></head><body>Text</body></html>", s, r), STLL::XhtmlException_c);
  // 2 heads
  BOOST_CHECK_THROW(layoutXHTML("<html><head></head><head></head><body><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // 2 bodies
  BOOST_CHECK_THROW(layoutXHTML("<html><head></head><body></body><body><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // table inside p tag
  BOOST_CHECK_THROW(layoutXHTML("<html><body><p><table></table>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // text inside ul but not li
  BOOST_CHECK_THROW(layoutXHTML("<html><body><ul>po</ul><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // text in table but not cell
  BOOST_CHECK_THROW(layoutXHTML("<html><body><table><colgroup></colgroup><tr>p</tr></table><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // text in colgroup
  BOOST_CHECK_THROW(layoutXHTML("<html><body><table><colgroup>asd</colgroup></table><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // text direclty in table
  BOOST_CHECK_THROW(layoutXHTML("<html><body><table><colgroup></colgroup>asd</table><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // table without colgroup
  BOOST_CHECK_THROW(layoutXHTML("<html><body><table><tr></tr></table><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
  // table with column group but invalid span value
  BOOST_CHECK_THROW(layoutXHTML("<html><body><table><colgroup><col span='0' /></colgroup></table><p>Text</p></body></html>", s, r), STLL::XhtmlException_c);
}

BOOST_AUTO_TEST_CASE( Simple_Layouts )
{
  auto c = std::make_shared<STLL::fontCache_c>();
  STLL::textStyleSheet_c s(c);

  s.font("sans", STLL::fontResource_c("tests/FreeSans.ttf"));
  s.addRule("body", "font-size", "16px");
  s.addRule("body", "color", "#ffffff");

  BOOST_CHECK(layouts_identical(STLL::layoutXHTML("<html><body><p lang='en'>Test Text</p></body></html>", s, STLL::rectangleShape_c(1000*64)), "tests/simple-01.lay", c));
}
