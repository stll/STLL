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
      if (l.data[i].command != STLL::textLayout_c::commandData::CMD_RECT) return false;
      if (l.data[i].x != std::stoi(a.attribute("x").value())) return false;
      if (l.data[i].y != std::stoi(a.attribute("y").value())) return false;
      if (l.data[i].w != std::stoi(a.attribute("w").value())) return false;
      if (l.data[i].h != std::stoi(a.attribute("h").value())) return false;
      if (l.data[i].c.r() != std::stoi(a.attribute("r").value())) return false;
      if (l.data[i].c.g() != std::stoi(a.attribute("g").value())) return false;
      if (l.data[i].c.b() != std::stoi(a.attribute("b").value())) return false;
      if (l.data[i].c.a() != std::stoi(a.attribute("a").value())) return false;
    }
    else if (a.name() == std::string("image"))
    {
      if (l.data[i].command != STLL::textLayout_c::commandData::CMD_IMAGE) return false;
      if (l.data[i].x != std::stoi(a.attribute("x").value())) return false;
      if (l.data[i].y != std::stoi(a.attribute("y").value())) return false;
      if (l.data[i].imageURL != a.attribute("url").value()) return false;
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
  s.addRule(".font", "font-family", "snas");

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
  // invalid tag in flow context
  BOOST_CHECK_THROW(layoutXHTML("<html><body><colgroup /></body></html>", s, r), STLL::XhtmlException_c);
  // invalid / undefined font
  s.addRule("p", "color", "#FFFFFF");
  BOOST_CHECK_THROW(layoutXHTML("<html><body><p class='font'>Test</p></body></html>", s, r), STLL::XhtmlException_c);

  // put something undefined in prasing context
  BOOST_CHECK_THROW(layoutXHTML("<html><body><p>Test<i><div /></i></p></body></html>", s, r), STLL::XhtmlException_c);

  // image with non-pixel size attrib
  BOOST_CHECK_THROW(layoutXHTML(
    "<html><body><p lang='en'>Te<img width='10' height='10' src='a' />st</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), STLL::XhtmlException_c);

  // relative font size and no parent for
  s.addRule("p", "font-size", "80%");
  BOOST_CHECK_THROW(layoutXHTML(
    "<html><body><p lang='en'>Test</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), STLL::XhtmlException_c);
}

BOOST_AUTO_TEST_CASE( Simple_Layouts )
{
  auto c = std::make_shared<STLL::fontCache_c>();
  STLL::textStyleSheet_c s(c);

  s.font("sans", STLL::fontResource_c("tests/FreeSans.ttf"));
  s.addRule("body", "font-size", "16px");
  s.addRule("body", "color", "#ffffff");

  // simple text
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-01.lay", c));

  // check text indent
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-02.lay", c));
  s.addRule("body", "text-indent", "0px");

  // a language with script and at the same time right to left: arabic
  s.font("sans-ar", STLL::fontResource_c("tests/Amiri.ttf"));
  s.addRule("p[lang|=ar]", "direction", "rtl");
  s.addRule("p[lang|=ar]", "font-family", "sans-ar");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='ar-arab'>كأس الأمم</p></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-03.lay", c));

  // usage of soft hyphen
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test&#173;Text&#173;Textwithaverylongadditiontomakeitlong</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-04.lay", c));

  // a simple inlay
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Te<img width='10px' height='10px' src='a' />st</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-05.lay", c));

  // a simple underline test
  s.addRule("p", "text-decoration", "underline");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-06.lay", c));
  s.addRule("p", "text-decoration", "");

  // more complex underline with inlay and only partially
  s.addRule("span", "text-decoration", "underline");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>T<span>e<img width='10px' height='10px' src='a' />s</span>t</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-07.lay", c));
  s.addRule("span", "text-decoration", "");

  // more complex underline with inlay and only partially and additionally a shadow
  s.addRule("body", "padding", "3px");  // make a bit space around to actually see something
  s.addRule("span", "text-decoration", "underline");
  s.addRule("p", "text-shadow", "1px 1px #FF0000, -1px -1px #00FF00");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>T<span>e<img width='10px' height='10px' src='a' />s</span>t</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-08.lay", c));
  s.addRule("span", "text-decoration", "");
  s.addRule("p", "text-shadow", "");

  // a simple underline test with spaces to underline
  s.addRule("p", "text-decoration", "underline");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Text with spaces</p></body></html>",
    s, STLL::rectangleShape_c(300*64)), "tests/simple-09.lay", c));
  s.addRule("p", "text-decoration", "");

  // centered text
  s.addRule("p", "text-align", "center");
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer text that will give us some lines of text in the output</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-10.lay", c));

  // left adjusted text
  s.addRule("p", "text-align", "left");
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer text that will give us some lines of text in the output</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-11.lay", c));

  // right adjusted text
  s.addRule("p", "text-align", "right");
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer text that will give us some lines of text in the output</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-12.lay", c));

  // justified text
  s.addRule("p", "text-align", "justify");
  s.addRule("p", "text-align-last", "left");
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer text that will give us some lines of text in the output</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-13.lay", c));

  // justified text
  s.addRule("p", "text-align", "justify");
  s.addRule("p", "text-align-last", "right");
  s.addRule("body", "text-indent", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer text that will give us some lines of text in the output</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-14.lay", c));
  s.addRule("p", "text-align", "left");
  s.addRule("p", "text-align-last", "");
  s.addRule("body", "text-indent", "0px");

  // must break
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer<br />text<br />that will give<br />us some lines<br />of text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-15.lay", c));

  // must break with centered text
  s.addRule("p", "text-align", "center");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p>A longer<br />text<br />that will give<br />us some lines<br />of text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-16.lay", c));
  s.addRule("p", "text-align", "left");

  // test of division with a style
  s.addRule(".tt", "color", "#80FF80");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><div class='tt'><p>Test Text</p></div></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-17.lay", c));

  // unordered list
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><ul><li>Test Text</li><li>More Text</li></ul></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-18.lay", c));

  // nested unordered list
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><ul><li>Test Text<ul><li>One nested item</li><li>And another one</li>"
    "</ul></li><li>More Text</li></ul></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-19.lay", c));

  // nested unordered list right to left
  s.addRule(".rtl", "direction", "rtl");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><ul class='rtl'><li>Test Text<ul><li>One nested item</li><li>And another one</li>"
    "</ul></li><li>More Text</li></ul></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-20.lay", c));

  // justified text with no align last value -> information comes from direction
  s.addRule("p", "text-align", "justify");
  s.addRule("p", "text-align-last", "");
  s.addRule("body", "text-indent", "10px");
  s.addRule(".rtl", "direction", "rtl");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body>"
    "<p>A longer text that will give us some lines of text in the output</p>"
    "<p class='rtl'>A longer text that will give us some lines of text in the output</p>"
    "</body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-21.lay", c));

  // no text align at all -> comes from direction
  s.addRule("p", "text-align", "");
  s.addRule("body", "text-indent", "10px");
  s.addRule(".rtl", "direction", "rtl");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body>"
    "<p>A longer text that will give us some lines of text in the output</p>"
    "<p class='rtl'>A longer text that will give us some lines of text in the output</p>"
    "</body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-22.lay", c));

  // sub and sup
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test<sub>1</sub> Text<sup>2</sup></p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-23.lay", c));

  // sub and sup with relatively smaller font size
  s.addRule("sub", "font-size", "70%");
  s.addRule("sup", "font-size", "50%");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test<sub>1</sub> Text<sup>2</sup></p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/simple-24.lay", c));

  // check HTML normalization: all paragraphs must look the same
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body>"
    "<p>Test Text</p>"
    "<p>Test  Text</p>"
    "<p>Test       Text</p>"
    "<p>Test\nText</p>"
    "<p>Test \n Text</p>"
    "</body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/simple-25.lay", c));
}

BOOST_AUTO_TEST_CASE( Table_Layouts )
{
  auto c = std::make_shared<STLL::fontCache_c>();
  STLL::textStyleSheet_c s(c);

  s.font("sans", STLL::fontResource_c("tests/FreeSans.ttf"));
  s.addRule("body", "font-size", "16px");
  s.addRule("body", "color", "#ffffff");
  s.addRule(".tc", "width", "100px");

  // basic table with 2x2 cells
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td>Test</td><td>T</td></tr><tr><td>T</td><td>Table</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/table-01.lay", c));

  // basic table with 2x2 cells, right to left direction
  s.addRule("table", "direction", "rtl");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td>Test</td><td>T</td></tr><tr><td>T</td><td>Table</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/table-02.lay", c));
  s.addRule("table", "direction", "ltr");

  // basic table with 2x2 cells with one multi line cell
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td>Test with some more text</td><td>T</td></tr><tr><td>T</td><td>Table</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/table-03.lay", c));

  // basic table with 2x2 cells with one multi line cell
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td rowspan='2'>Test with some more text</td><td>T</td></tr><tr><td>Table</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/table-04.lay", c));

  // basic table with 2x2 cells with one multi columns cell
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td colspan='2'>Test with some more text</td></tr><tr><td>T</td><td>Table</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/table-05.lay", c));
}

BOOST_AUTO_TEST_CASE( Frames_Layouts )
{
  auto c = std::make_shared<STLL::fontCache_c>();
  STLL::textStyleSheet_c s(c);

  s.font("sans", STLL::fontResource_c("tests/FreeSans.ttf"));

  s.addRule("body", "font-size", "16px");
  s.addRule("body", "color", "#FFFFFF");
  s.addRule("body", "border-color", "#FFFF00");
  s.addRule("body", "background-color", "#000040");

  // top border
  s.addRule("body", "border-top-width", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/border-01.lay", c));
  s.addRule("body", "border-top-width", "0px");

  // bottom border
  s.addRule("body", "border-bottom-width", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/border-02.lay", c));
  s.addRule("body", "border-bottom-width", "0px");

  // left border
  s.addRule("body", "border-left-width", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/border-03.lay", c));
  s.addRule("body", "border-left-width", "0px");

  // right border
  s.addRule("body", "border-right-width", "10px");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/border-04.lay", c));
  s.addRule("body", "border-right-width", "0px");

  // check margin collapsing
  s.addRule("p", "margin", "10px");
  s.addRule("p", "margin-bottom", "5px");
  s.addRule("body", "margin", "2px");
  s.addRule("p", "border-width", "1px");
  s.addRule("p", "border-color", "#FFFF00");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><p lang='en'>Test Text</p><p>Text Test</p></body></html>",
    s, STLL::rectangleShape_c(200*64)), "tests/border-05.lay", c));
  s.addRule("p", "border-width", "0px");

  // table with 2x3 cells with one multiline and one multirow cell with bordercollapse
  s.addRule(".tc", "width", "100px");
  s.addRule("td", "border-width", "1px");
  s.addRule("table", "border-collapse", "collapse");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td colspan='2'>Test with some more text</td></tr><tr><td rowspan='2'>T</td><td>Table</td></tr><tr><td>Oops</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/border-06.lay", c));

  // table with 2x3 cells with one multiline and one multirow cell with bordercollapse but margins and extra right margin
  s.addRule(".tc", "width", "100px");
  s.addRule("td", "border-width", "1px");
  s.addRule("td", "margin", "1px");
  s.addRule("td", "border-right-width", "2px");
  s.addRule("td", "margin-right", "2px");
  s.addRule("table", "border-collapse", "collapse");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td colspan='2'>Test with some more text</td></tr><tr><td rowspan='2'>T</td><td>Table</td></tr><tr><td>Oops</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/border-07.lay", c));

  // table with 2x3 cells with one multiline and one multirow cell with bordercollapse not margins, extra right border
  s.addRule(".tc", "width", "100px");
  s.addRule("td", "border-width", "1px");
  s.addRule("td", "margin", "0px");
  s.addRule("td", "border-right-width", "2px");
  s.addRule("td", "border-bottom-width", "2px");
  s.addRule("td", "margin-right", "0px");
  s.addRule("table", "border-collapse", "collapse");
  BOOST_CHECK(layouts_identical(STLL::layoutXHTML(
    "<html><body><table><colgroup><col class='tc' /><col class='tc' /></colgroup>"
    "<tr><td colspan='2'>Test with some more text</td></tr><tr><td rowspan='2'>T</td><td>Table</td></tr><tr><td>Oops</td></tr></table></body></html>",
    s, STLL::rectangleShape_c(1000*64)), "tests/border-08.lay", c));
}