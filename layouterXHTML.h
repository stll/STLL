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
#ifndef STLL_LAYOUTER_XHTML_H
#define STLL_LAYOUTER_XHTML_H

/** \file
 *  \brief This module contains the XHTML parser and layouter
 */

#include "layouterCSS.h"
#include "layouter.h"

#include "xmllibraries.h"
#include "utf-8.h"

#include <string>

namespace STLL {

  namespace internal {

template <class X>
class tableCell
{
  public:
    uint32_t row;
    uint32_t col;
    uint32_t rowspan;
    uint32_t colspan;

    X xml;

    TextLayout_c l;
};

template <class T>
class vector2d {
  private:
    std::vector<std::vector<T> > data;
    T def;
  public:

    vector2d(void) : def(0) {}

    void set(size_t x, size_t y, const T & val) {
      if (data.size() <= y) data.resize(y+1);
      if (data[y].size() <= x) data[y].resize(x+1);
      data[y][x] = val;
    }

    const T & get(size_t x, size_t y) const
    {
      if (y >= data.size() || x >= data[y].size()) return def;
      return data[y][x];
    }

    T & get(size_t x, size_t y)
    {
      if (y >= data.size() || x >= data[y].size()) return def;
      return data[y][x];
    }

    void rectangularize(void)
    {
      data.resize(data.size()+1);
      size_t m = 0;

      for (const auto & a : data)
        m = std::max(m, a.size());

      m++;

      for (auto & a : data)
        a.resize(m);
    }
};

class indentShape_c : public Shape_c
{
  private:
    const Shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    indentShape_c(const Shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_right; }
    virtual int32_t getLeft2(int32_t top, int32_t bottom) const { return outside.getLeft2(top, bottom)+ind_left; }
    virtual int32_t getRight2(int32_t top, int32_t bottom) const { return outside.getRight2(top, bottom)-ind_right; }
};

class stripLeftShape_c : public Shape_c
{
  private:
    const Shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    stripLeftShape_c(const Shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_right; }
    virtual int32_t getLeft2(int32_t top, int32_t bottom) const { return outside.getLeft2(top, bottom)+ind_left; }
    virtual int32_t getRight2(int32_t top, int32_t bottom) const { return outside.getLeft2(top, bottom)+ind_right; }
};

class stripRightShape_c : public Shape_c
{
  private:
    const Shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    stripRightShape_c(const Shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_right; }
    virtual int32_t getLeft2(int32_t top, int32_t bottom) const { return outside.getRight2(top, bottom)-ind_left; }
    virtual int32_t getRight2(int32_t top, int32_t bottom) const { return outside.getRight2(top, bottom)-ind_right; }
};


color_c evalColor(const std::string & col);
std::vector<codepointAttributes::shadow> evalShadows(const std::string & v);
std::string normalizeHTML(const std::string & in, char prev);

class szFunctor
{
  public:
    virtual double operator()(void) const { return 0; };
};

template <class X>
class parentFunctor : public szFunctor
{
  private:
    std::string tag;
    X node;
    const textStyleSheet_c & rules;

  public:
    parentFunctor(const std::string & t, X n, const textStyleSheet_c & r) :
       tag(t), node(n), rules(r) {}

    double operator()(void) const
    {
      if (!node)
        throw XhtmlException_c("no parent node to base a percent value on");

      parentFunctor f(tag, xml_getParent(node), rules);
      return evalSize(rules.getValue(node, tag), f);
    }
};


/** \brief evaluate size
 *  \param sz the size string from the CSS
 *  \return the resulting size in pixel
 */
template <class T = szFunctor>
static double evalSize(const std::string & sz, T f = szFunctor())
{
  // right now we accept only pixel sizes
  size_t l = sz.length();

  if (sz[l-2] == 'p' && sz[l-1] == 'x')
  {
    return 64*atof(sz.c_str());
  }
  else if (sz[l-1] == '%')
  {
    return f() * atof(sz.c_str()) / 100;
  }

  throw XhtmlException_c("only pixel size format is supported");

  return 0;
}


template <class X>
std::string getNodePath(X xml)
{
  if (xml_isEmpty(xml))
    return "";
  else
    return getNodePath(xml_getParent(xml)) + "/" + xml_getName(xml);
}



template <class X>
std::shared_ptr<FontFace_c> getFontForNode(X xml, const textStyleSheet_c & rules)
{
  std::string fontFamily = rules.getValue(xml, "font-family");
  std::string fontStyle = rules.getValue(xml, "font-style");
  std::string fontVariant = rules.getValue(xml, "font-variant");
  std::string fontWeight = rules.getValue(xml, "font-weight");

  parentFunctor<X> fkt("font-size", xml_getParent(xml), rules);
  double fontSize = evalSize(rules.getValue(xml, "font-size"), fkt);

  auto fam = rules.findFamily(fontFamily);

  if (fam)
  {
    auto f = fam->getFont(fontSize, fontStyle, fontVariant, fontWeight);

    if (f) return f;
  }

  throw XhtmlException_c(std::string("Requested font not found (family:'") + fontFamily +
                                     "', style: '" + fontStyle +
                                     "', variant: '" + fontVariant +
                                     "', weight: '" + fontWeight + ") required here: " + getNodePath(xml));

  return 0;
}


template <class X>
const char * getHTMLAttribute(X xml, const std::string attr)
{
  while (true)
  {
    const char * res = xml_getAttribute(xml, attr.c_str());

    if (res && *res) return res;

    if ("lang" == attr)
    {
      xml = xml_getParent(xml);

      if (xml_isEmpty(xml))
      {
        return "";
      }
    }
    else
    {
      return "";
    }
  }
}



template <class X>
using ParseFunction = TextLayout_c (*)(X & xml, const textStyleSheet_c & rules,
                                      const Shape_c & shape, int32_t ystart);

// handles padding, margin and border, all in one, it takes the text returned from the
// ParseFunction and boxes it
template <class X>
TextLayout_c boxIt(X & xml, X & xml2, const textStyleSheet_c & rules,
                          const Shape_c & shape, int32_t ystart, ParseFunction<X> fkt,
                          X above, X left,
                          bool collapseBorder = false, uint32_t minHeight = 0)
{
  int32_t padding_left = 0;
  int32_t padding_right = 0;
  int32_t padding_top = 0;
  int32_t padding_bottom = 0;

  padding_left = padding_right = padding_top = padding_bottom = evalSize(rules.getValue(xml, "padding"));

  if (rules.getValue(xml, "padding-left") != "")   padding_left   = evalSize(rules.getValue(xml, "padding-left"));
  if (rules.getValue(xml, "padding-right") != "")  padding_right  = evalSize(rules.getValue(xml, "padding-right"));
  if (rules.getValue(xml, "padding-top") != "")    padding_top    = evalSize(rules.getValue(xml, "padding-top"));
  if (rules.getValue(xml, "padding-bottom") != "") padding_bottom = evalSize(rules.getValue(xml, "padding-bottom"));

  int32_t borderwidth_left = 0;
  int32_t borderwidth_right = 0;
  int32_t borderwidth_top = 0;
  int32_t borderwidth_bottom = 0;

  borderwidth_left = borderwidth_right = borderwidth_top = borderwidth_bottom = evalSize(rules.getValue(xml, "border-width"));

  if (rules.getValue(xml, "border-left-width") != "")   borderwidth_left   = evalSize(rules.getValue(xml, "border-left-width"));
  if (rules.getValue(xml, "border-right-width") != "")  borderwidth_right  = evalSize(rules.getValue(xml, "border-right-width"));
  if (rules.getValue(xml, "border-top-width") != "")    borderwidth_top    = evalSize(rules.getValue(xml, "border-top-width"));
  if (rules.getValue(xml, "border-bottom-width") != "") borderwidth_bottom = evalSize(rules.getValue(xml, "border-bottom-width"));

  int32_t margin_left = 0;
  int32_t margin_right = 0;
  int32_t margin_top = 0;
  int32_t margin_bottom = 0;

  margin_left = margin_right = margin_top = margin_bottom = evalSize(rules.getValue(xml, "margin"));

  if (rules.getValue(xml, "margin-left") != "")   margin_left   = evalSize(rules.getValue(xml, "margin-left"));
  if (rules.getValue(xml, "margin-right") != "")  margin_right  = evalSize(rules.getValue(xml, "margin-right"));
  if (rules.getValue(xml, "margin-top") != "")    margin_top    = evalSize(rules.getValue(xml, "margin-top"));
  if (rules.getValue(xml, "margin-bottom") != "") margin_bottom = evalSize(rules.getValue(xml, "margin-bottom"));

  int32_t marginElementAbove = 0;
  int32_t marginElementLeft = 0;
  int32_t borderElementAbove = 0;
  int32_t borderElementLeft = 0;

  if (!xml_isEmpty(above))
  {
    marginElementAbove = evalSize(rules.getValue(above, "margin"));
    if (rules.getValue(above, "margin-bottom") != "")
      marginElementAbove = evalSize(rules.getValue(above, "margin-bottom"));

    if (margin_top == 0 && marginElementAbove == 0)
    {
      borderElementAbove = evalSize(rules.getValue(above, "border-width"));
      if (rules.getValue(above, "border-bottom-width") != "")
        borderElementAbove = evalSize(rules.getValue(above, "border-bottom-width"));
    }
  }

  if (!xml_isEmpty(left))
  {
    marginElementLeft = evalSize(rules.getValue(left, "margin"));
    if (rules.getValue(left, "margin-right") != "")
      marginElementLeft = evalSize(rules.getValue(left, "margin-right"));

    if (margin_left == 0 && marginElementLeft == 0)
    {
      borderElementLeft = evalSize(rules.getValue(left, "border-width"));
      if (rules.getValue(left, "border-right-width") != "")
        borderElementLeft = evalSize(rules.getValue(left, "border-right-width"));
    }
  }

  margin_top = std::max(marginElementAbove, margin_top)-marginElementAbove;
  margin_left = std::max(marginElementLeft, margin_left)-marginElementLeft;

  if (collapseBorder)
  {
    borderwidth_top =  std::max(borderElementAbove, borderwidth_top)-borderElementAbove;
    borderwidth_left = std::max(borderElementLeft, borderwidth_left)-borderElementLeft;
  }

  auto l2 = fkt(xml2, rules,
                indentShape_c(shape, padding_left+borderwidth_left+margin_left, padding_right+borderwidth_right+margin_right),
                ystart+padding_top+borderwidth_top+margin_top);

  int space = minHeight - (l2.getHeight()+padding_bottom+borderwidth_bottom+margin_bottom);
  l2.setHeight(std::max(minHeight, l2.getHeight()+padding_bottom+borderwidth_bottom+margin_bottom));

  if (space > 0)
  {
    // TODO baseline is missing
         if (rules.getValue(xml, "vertical-align") == "bottom") l2.shift(0, space);
    else if (rules.getValue(xml, "vertical-align") == "middle") l2.shift(0, space/2);
  }

  if (borderwidth_top)
  {
    std::string color = rules.getValue(xml, "border-color");
    if (rules.getValue(xml, "border-top-color") != "") color = rules.getValue(xml, "border-top-color");
    if (color == "")                                   color = rules.getValue(xml, "color");
    auto cc = evalColor(color);

    if (cc.a() != 0)
    {
      int32_t cx = l2.getLeft()-padding_left-borderwidth_left;
      int32_t cy = ystart+margin_top;
      int32_t cw = l2.getRight()-l2.getLeft()+padding_left+padding_right+borderwidth_left+borderwidth_right;
      int32_t ch = borderwidth_top;
      l2.addCommandStart(cx, cy, cw, ch, cc);
    }
  }

  if (borderwidth_bottom)
  {
    std::string color = rules.getValue(xml, "border-color");
    if (rules.getValue(xml, "border-bottom-color") != "") color = rules.getValue(xml, "border-bottom-color");
    if (color == "")                                      color = rules.getValue(xml, "color");
    auto cc = evalColor(color);

    if (cc.a() != 0)
    {
      int32_t cx = l2.getLeft()-padding_left-borderwidth_left;
      int32_t cy = l2.getHeight()-borderwidth_bottom-margin_bottom;
      int32_t cw = l2.getRight()-l2.getLeft()+padding_left+padding_right+borderwidth_left+borderwidth_right;
      int32_t ch = borderwidth_bottom;
      l2.addCommandStart(cx, cy, cw, ch, cc);
    }
  }

  if (borderwidth_right)
  {
    std::string color = rules.getValue(xml, "border-color");
    if (rules.getValue(xml, "border-right-color") != "") color = rules.getValue(xml, "border-right-color");
    if (color == "")                                     color = rules.getValue(xml, "color");
    auto cc = evalColor(color);

    if (cc.a() != 0)
    {
      int32_t cx = l2.getRight()+padding_right;
      int32_t cy = ystart+margin_top;
      int32_t cw = borderwidth_right;
      int32_t ch = l2.getHeight()-ystart-margin_bottom-margin_top;
      l2.addCommandStart(cx, cy, cw, ch, cc);
    }
  }

  if (borderwidth_left)
  {
    std::string color = rules.getValue(xml, "border-color");
    if (rules.getValue(xml, "border-left-color") != "") color = rules.getValue(xml, "border-left-color");
    if (color == "")                                    color = rules.getValue(xml, "color");
    auto cc = evalColor(color);

    if (cc.a() != 0)
    {
      int32_t cx = l2.getLeft()-padding_left-borderwidth_left;
      int32_t cy = ystart+margin_top;
      int32_t cw = borderwidth_left;
      int32_t ch = l2.getHeight()-ystart-margin_bottom-margin_top;
      l2.addCommandStart(cx, cy, cw, ch, cc);
    }
  }

  auto cc = evalColor(rules.getValue(xml, "background-color"));

  if (cc.a() != 0)
  {
    int32_t cx = shape.getLeft(ystart+margin_top, ystart+margin_top)+borderwidth_left+margin_left;
    int32_t cy = ystart+borderwidth_top+margin_top;
    int32_t cw = shape.getRight(ystart+margin_top, ystart+margin_top)-
                 shape.getLeft(ystart+margin_top, ystart+margin_top)-borderwidth_right-borderwidth_left-margin_right-margin_left;
    int32_t ch = l2.getHeight()-ystart-borderwidth_bottom-borderwidth_top-margin_bottom-margin_top;
    l2.addCommandStart(cx, cy, cw, ch, cc);
  }

#ifdef _DEBUG_ // allows to see the boxes using a random color for each

  textLayout_c::commandData c;
  c.command = textLayout_c::commandData::CMD_RECT;
  c.x = shape.getLeft(ystart, ystart);
  c.y = ystart;
  c.w = shape.getRight(ystart, ystart)-shape.getLeft(ystart, ystart);
  c.h = l2.getHeight()-ystart;
  c.r = rand() % 128;
  c.g = rand() % 128;
  c.b = rand() % 128;
  c.a = 128;

  l2.addCommandStart(c);

#endif

  l2.setLeft(l2.getLeft()-padding_left-borderwidth_left-margin_left);
  l2.setRight(l2.getRight()+padding_right+borderwidth_right+margin_right);

  return l2;
}



template <class X>
TextLayout_c layoutXML_IMG(X & xml, const textStyleSheet_c & rules,
                                  const Shape_c & shape, int32_t ystart)
{
  TextLayout_c l;

  int32_t cx = shape.getLeft(ystart, ystart);
  int32_t cy = ystart;
  int32_t cw = evalSize(xml_getAttribute(xml, "width"));
  int32_t ch = evalSize(xml_getAttribute(xml, "height"));
  auto ci = xml_getAttribute(xml, "src");

  l.addCommand(ci, cx, cy, cw, ch);
  l.setHeight(ystart+ch);
  l.setLeft(cx);
  l.setRight(cx+cw);

  return l;
}


// this function is different from all the other layout functions as it
// will take the first XML-node to layout and work along with the siblings
// instead of looking at the children
// this function will also return a new node where it stopped working
template <class X>
X layoutXML_text(X xml, const textStyleSheet_c & rules,
                              layoutProperties & prop, std::u32string & txt,
                              AttributeIndex_c & attr, int32_t baseline = 0,
                              const std::string & link = "", bool exitOnError = false)
{
  while (!xml_isEmpty(xml))
  {
    if (xml_isDataNode(xml))
    {
      size_t s = txt.length();

      if (txt.length() == 0)
        txt = u8_convertToU32(normalizeHTML(xml_getData(xml), ' '));
      else
        txt += u8_convertToU32(normalizeHTML(xml_getData(xml), txt[txt.length()-1]));

      codepointAttributes a;

      a.c = evalColor(rules.getValue(xml_getParent(xml), "color"));
      a.font = getFontForNode(xml_getParent(xml), rules);
      a.lang = getHTMLAttribute(xml_getParent(xml), "lang");
      a.flags = 0;
      if (rules.getValue(xml_getParent(xml), "text-decoration") == "underline")
      {
        a.flags |= codepointAttributes::FL_UNDERLINE;
      }
      a.shadows = evalShadows(rules.getValue(xml_getParent(xml), "text-shadow"));

      a.baseline_shift = baseline;

      if (!link.empty())
      {
        prop.links.push_back(link);
        a.link = prop.links.size();
      }

      attr.set(s, txt.length()-1, a);
    }
    else if (   (xml_isElementNode(xml))
             && (   (std::string("i") == xml_getName(xml))
                 || (std::string("span") == xml_getName(xml))
                 || (std::string("b") == xml_getName(xml))
                 || (std::string("code") == xml_getName(xml))
                 || (std::string("em") == xml_getName(xml))
                 || (std::string("q") == xml_getName(xml))
                 || (std::string("small") == xml_getName(xml))
                 || (std::string("strong") == xml_getName(xml))
                 || (std::string("a") == xml_getName(xml))
                )
            )
    {
      if (rules.getValue(xml, "direction") == "rtl")
      {
        txt += U"\U0000202B";
      }
      else
      {
        txt += U"\U0000202A";
      }

      if (std::string("a") == xml_getName(xml))
      {
        layoutXML_text(xml_getFirstChild(xml), rules, prop, txt, attr, baseline, xml_getAttribute(xml, "href"));
      }
      else
      {
        layoutXML_text(xml_getFirstChild(xml), rules, prop, txt, attr, baseline, link);
      }
      txt += U"\U0000202C";
    }
    else if (xml_isElementNode(xml) && (std::string("sub") == xml_getName(xml)))
    {
      auto font = getFontForNode(xml, rules);

      layoutXML_text(xml_getFirstChild(xml), rules, prop, txt, attr, baseline-font->getAscender()/2, link);
    }
    else if (xml_isElementNode(xml) && (std::string("sup") == xml_getName(xml)))
    {
      auto font = getFontForNode(xml_getParent(xml), rules);

      layoutXML_text(xml_getFirstChild(xml), rules, prop, txt, attr, baseline+font->getAscender()/2, link);
    }
    else if (xml_isElementNode(xml) && (std::string("br") == xml_getName(xml)))
    {
      txt += U'\n';
      codepointAttributes a;
      a.flags = 0;
      a.font = getFontForNode(xml_getParent(xml), rules);
      a.lang = getHTMLAttribute(xml_getParent(xml), "lang");
      attr.set(txt.length()-1, a);
    }
    else if (xml_isElementNode(xml) && (std::string("img") == xml_getName(xml)))
    {
      codepointAttributes a;
      a.inlay = std::make_shared<TextLayout_c>(boxIt(xml, xml, rules, RectangleShape_c(10000), 0,
                                                     layoutXML_IMG, X(), X()));
      a.baseline_shift = 0;
      a.shadows = evalShadows(rules.getValue(xml_getParent(xml), "text-shadow"));

      // if we want underlines, we add the font so that the layouter
      // can find the position of the underline
      if (rules.getValue(xml_getParent(xml), "text-decoration") == "underline")
      {
        a.flags |= codepointAttributes::FL_UNDERLINE;
        a.font = getFontForNode(xml_getParent(xml), rules);
        a.c = evalColor(rules.getValue(xml_getParent(xml), "color"));
      }

      if (!link.empty())
      {
        prop.links.push_back(link);
        a.link = prop.links.size();
      }

      txt += U'\u00A0';
      attr.set(txt.length()-1, a);
    }
    else
    {
      if (exitOnError)
        break;
      else
        throw XhtmlException_c("Found non phrasing element in phrasing context (" + getNodePath(xml) + ")" );
    }

    xml = xml_getNextSibling(xml);
  }
  return xml;
}




// this function is different from all other layout functions usable in the boxIt
// function, as it will change the xml node and return a new one
template <class X>
TextLayout_c layoutXML_Phrasing(X & xml, const textStyleSheet_c & rules, const Shape_c & shape, int32_t ystart)
{
  std::u32string txt;
  AttributeIndex_c attr;
  layoutProperties lprop;

  auto xml2 = layoutXML_text(xml, rules, lprop, txt, attr, 0, "", true);

  std::string s = rules.getValue(xml, "text-align");

  if      (s == "left")   lprop.align = layoutProperties::ALG_LEFT;
  else if (s == "right")  lprop.align = layoutProperties::ALG_RIGHT;
  else if (s == "center") lprop.align = layoutProperties::ALG_CENTER;
  else if (s == "justify") {
    s = rules.getValue(xml, "text-align-last");
    if      (s == "left")  lprop.align = layoutProperties::ALG_JUSTIFY_LEFT;
    else if (s == "right") lprop.align = layoutProperties::ALG_JUSTIFY_RIGHT;
    else if (s == "")
    {
      s = rules.getValue(xml, "direction");
      if (s == "ltr") lprop.align = layoutProperties::ALG_JUSTIFY_LEFT;
      if (s == "rtl") lprop.align = layoutProperties::ALG_JUSTIFY_RIGHT;
    }
    else
    {
      throw XhtmlException_c("Only 'left' and 'right' are allowed as values for the "
                             "'text-align-last' CSS property (" + getNodePath(xml) + ")");
    }
  }
  else if (s == "")
  {
    s = rules.getValue(xml, "direction");
    if (s == "ltr") lprop.align = layoutProperties::ALG_LEFT;
    if (s == "rtl") lprop.align = layoutProperties::ALG_RIGHT;
  }
  else
  {
    throw XhtmlException_c("Only 'left, 'right', 'center' and 'justify' are allowed for "
                           "the 'text-align' CSS property (" + getNodePath(xml) + ")");
  }

  lprop.indent = evalSize(rules.getValue(xml, "text-indent"));
  lprop.ltr = rules.getValue(xml, "direction") == "ltr";
  lprop.round = rules.getRound();
  lprop.underlineFont = getFontForNode(xml_getParent(xml), rules);

  xml = xml2;

  return layoutParagraph(txt, attr, shape, lprop, ystart);
}




template <class X>
TextLayout_c layoutXML_Flow(X & txt, const textStyleSheet_c & rules, const Shape_c & shape, int32_t ystart);

template <class X>
TextLayout_c layoutXML_UL(X & xml, const textStyleSheet_c & rules, const Shape_c & shape, int32_t ystart)
{
  TextLayout_c l;
  l.setHeight(ystart);
  xml_forEachChild(xml, [xml, &rules, &l, &shape, ystart](X i) -> bool {
    if (xml_isElementNode(i) && (std::string("li") == xml_getName(i)))
    {
      auto j = xml;
      while (xml_isElementNode(j))
        j = xml_getFirstChild(j);

      auto font = getFontForNode(i, rules);
      auto y = l.getHeight();

      codepointAttributes a;
      a.c = evalColor(rules.getValue(xml, "color"));
      a.font = font;
      a.lang = "";
      a.flags = 0;
      a.shadows = evalShadows(rules.getValue(xml, "text-shadow"));

      int32_t padding = evalSize(rules.getValue(i, "padding"));
      int32_t listIndent = font->getAscender();

      auto direction = rules.getValue(xml, "direction");

      layoutProperties prop;
      prop.indent = 0;
      prop.ltr = true;
      prop.align = layoutProperties::ALG_CENTER;
      prop.round = rules.getRound();

      std::unique_ptr<Shape_c> bulletshape;

      if (direction == "ltr")
      {
        bulletshape.reset(new stripLeftShape_c(shape, padding, padding+listIndent));
      }
      else
      {
        bulletshape.reset(new stripRightShape_c(shape, padding+listIndent, padding));
      }

      indentShape_c textshape(shape, direction == "ltr" ? listIndent : 0, direction == "ltr" ? 0: listIndent);

      TextLayout_c bullet = layoutParagraph(U"\u2022", AttributeIndex_c(a), *bulletshape.get(), prop, y+padding);
      TextLayout_c text = boxIt(i, i, rules, textshape, y, layoutXML_Flow, xml_getPreviousSibling(i), X());

      // append the bullet first and then the text, adjusting the bullet so that its baseline
      // is at the same vertical position as the first baseline in the text
      l.append(bullet, 0, text.getFirstBaseline() - bullet.getFirstBaseline());
      l.append(text);

      l.setLeft(shape.getLeft2(ystart, l.getHeight()));
      l.setRight(shape.getRight2(ystart, l.getHeight()));
    }
    else
    {
      throw XhtmlException_c("Only 'li' tags allowed within 'ul' tag (" + getNodePath(i) + ")");
    }
    return false;
  });

  return l;
}




template <class X>
void layoutXML_TR(X & xml, uint32_t row, const textStyleSheet_c & /* rules */,
                         std::vector<tableCell<X>> & cells, vector2d<X> & cellarray,
                         size_t columns)
{
  uint32_t col = 0;
  while (!xml_isEmpty(cellarray.get(col+1, row+1))) col++;

  // collect all cells of the row
  xml_forEachChild(xml, [&cells, &cellarray, &col, row, columns](X i) -> bool {
    if (   xml_isElementNode(i)
        && (   (std::string("th") == xml_getName(i))
            || (std::string("td") == xml_getName(i))
           )
       )
    {
      tableCell<X> c;

      c.xml = i;
      c.rowspan = 1;
      c.colspan = 1;

      xml_forEachAttribute(i, [&c](const char * n, const char * v) -> bool {
        if (std::string("rowspan") == n)
        {
          c.rowspan = std::stoi(v);
        }
        if (std::string("colspan") == n)
        {
          c.colspan = std::stoi(v);
        }
        return false;
      });

      c.row = row;
      c.col = col;

      cells.emplace_back(c);

      for (uint32_t x = col; x < col+c.colspan; x++)
        for (uint32_t y = row; y < row+c.rowspan; y++)
          cellarray.set(x+1, y+1, i);

      col += c.colspan;

      if (col > columns)
      {
        throw XhtmlException_c("You must not use more columns that specified in the colgroup tag (" + getNodePath(i) + ")");
      }
    }
    else
    {
      throw XhtmlException_c("Only 'th' or 'td' tags allowed within 'ul' tag (" + getNodePath(i) + ")");
    }
    return false;
  });
}





template <class X>
TextLayout_c layoutXML_TABLE(X & xml, const textStyleSheet_c & rules, const Shape_c & shape, int32_t ystart)
{
  std::vector<tableCell<X>> cells;
  std::vector<uint32_t> widths;
  std::vector<uint32_t> colStart;
  vector2d<X> cellarray;
  uint32_t row = 0;
  bool col = false;
  bool rtl = (rules.getValue(xml, "direction") == "rtl");
  int left = rtl ? 1 : -1;

  std::string defaulttablewidth("100%");
  auto tablew = rules.getValue(xml, "width", defaulttablewidth);
  double table_width = evalSize(tablew, [&shape, ystart] (void)->double {
    return shape.getRight(ystart, ystart) - shape.getLeft(ystart, ystart);
  });

  // collect all cells of the table
  for (auto i = xml_getFirstChild(xml); !xml_isEmpty(i); i = xml_getNextSibling(i))
  {
    if (   (xml_isElementNode(i))
        && (std::string("colgroup") == xml_getName(i))
       )
    {
      std::vector<double> relativeWidths;

      for (auto j = xml_getFirstChild(i); !xml_isEmpty(j); j = xml_getNextSibling(j))
      {
        if (   (xml_isElementNode(j))
            && (std::string("col") == xml_getName(j))
           )
        {
          int span = 1;
          auto a = xml_getAttribute(j, "span");
          if (a)
          {
            span = std::stoi(a);
          }

          if (span == 0)
          {
            throw XhtmlException_c("malformed 'span' attribute (" + getNodePath(j) + ")");
          }

          auto w = rules.getValue(j, "width");

          if (w.back() == '*')
          {
            double width = std::stod(w.substr(0, w.length()-1).c_str());

            while (span > 0)
            {
              widths.push_back(0);
              relativeWidths.push_back(width);
              span--;
            }
          }
          else
          {
            uint32_t width = evalSize(w, [table_width] (void) {
              return table_width;
            });

            while (span > 0)
            {
              widths.push_back(width);
              relativeWidths.push_back(0);
              span--;
            }
          }
        }
        else
        {
          throw XhtmlException_c("Only 'col' tags allowed within 'colgroup' tag (" + getNodePath(j) + ")");
        }
      }

      assert(relativeWidths.size() == widths.size());

      // handle relative widths
      // 1) calculate the remaining width available for the table
      uint32_t remainingWidth = 0;
      for (auto w : widths)
        remainingWidth += w;

      if (remainingWidth < table_width)
      {
        remainingWidth = table_width - remainingWidth;

        // 2) calculate the sum of all relative widths
        double relSum = 0.0;
        for (auto w : relativeWidths)
          relSum += w;

        // 3) distribute the space
        if (relSum > 0.01)
        {
          for (int i = 0; i < relativeWidths.size(); i++)
          {
            widths[i] += remainingWidth * relativeWidths[i]/relSum;
          }
        }
      }

      col = true;
    }
    else if (xml_isElementNode(i) && (std::string("tr") == xml_getName(i))
       )
    {
      if (!col)
      {
        throw XhtmlException_c("You must define columns and widths in a table (" + getNodePath(i) + ")");
      }

      layoutXML_TR(i, row, rules, cells, cellarray, widths.size());
      row++;
    }
    else
    {
      throw XhtmlException_c("Only 'tr' and 'colgroup' tags allowed within 'table' tag (" + getNodePath(i) + ")");
    }
  }

  // make cellarray contain one extra row and column to get the neighbors easily
  cellarray.rectangularize();

  // calculate the start of each columns
  colStart.push_back(0);
  for (size_t i = 0; i < widths.size(); i++)
    colStart.push_back(*colStart.rbegin() + widths[i]);

  // make a first run over all cells to get a first minimal
  // hight for each cell
  for (auto & c : cells)
  {
    c.l = boxIt(c.xml, c.xml, rules, RectangleShape_c(colStart[c.col+c.colspan]-colStart[c.col]),
                0, layoutXML_Flow, cellarray.get(c.col+1, c.row), cellarray.get(c.col+(1+left)*c.colspan, c.row+1),
                rules.getValue(xml, "border-collapse") == "collapse");
  }

  // calculate the height of each row of the table by finding the cell with the maximal
  // height for each row
  // multi row cells only count for the final row
  uint32_t maxrow = 0;

  for (auto & c : cells)
  {
    maxrow = std::max(maxrow, c.row+c.rowspan);
  }

  std::vector<uint32_t> rowheights(maxrow);

  for (auto & c : cells)
  {
    if (c.rowspan == 1)
    {
      rowheights[c.row] = std::max(rowheights[c.row], c.l.getHeight());
    }
  }

  for (auto & c : cells)
  {
    if (c.rowspan > 1)
    {
      uint32_t h = 0;

      for (size_t r = c.row; r < c.row+c.rowspan; r++)
        h += rowheights[r];

      if (h < c.l.getHeight())
        rowheights[c.row+c.rowspan-1] += c.l.getHeight()-h;
    }
  }

  // find out the real width of the table
  int width = *colStart.rbegin();

  // place table into the center... TODO we may not always want to do that
  int xindent = 0;

  xindent = shape.getLeft(ystart, ystart) + (shape.getRight(ystart, ystart) - shape.getLeft(ystart, ystart) - width) / 2 ;

  if (xindent < 0) xindent = 0;

  // layout the table
  TextLayout_c l;
  row = 0;

  for (auto & c : cells)
  {
    if (row != c.row)
    {
      ystart += rowheights[row];
      row = c.row;
    }

    uint32_t rh = 0;
    for (size_t r = row; r < row+c.rowspan; r++)
      rh += rowheights[r];

    if (rh != c.l.getHeight())
      c.l = boxIt(c.xml, c.xml, rules, RectangleShape_c(colStart[c.col+c.colspan]-colStart[c.col]),
                  0, layoutXML_Flow, cellarray.get(c.col+1, c.row), cellarray.get(c.col+(1+left)*c.colspan, c.row+1),
                  rules.getValue(xml, "border-collapse") == "collapse", rh);

    if (l.getData().empty())
      l.setFirstBaseline(c.l.getFirstBaseline()+ystart);

    if (rtl)
    {
      l.append(c.l, xindent+*colStart.rbegin()-colStart[1]+colStart[0]-colStart[c.col+c.colspan-1], ystart);
    }
    else
    {
      l.append(c.l, colStart[c.col]+xindent, ystart);
    }
  }

  l.setHeight(ystart+rowheights[row]);
  l.setLeft(xindent+colStart[0]);
  l.setRight(xindent+width);

  return l;
}

template <class X>
TextLayout_c layoutXML_Flow(X & txt, const textStyleSheet_c & rules, const Shape_c & shape, int32_t ystart)
{
  TextLayout_c l;
  l.setHeight(ystart);

  auto i = xml_getFirstChild(txt);

  while (!xml_isEmpty(i))
  {
    if (   (xml_isElementNode(i))
        && (   (std::string("p") ==  xml_getName(i))
            || (std::string("h1") == xml_getName(i))
            || (std::string("h2") == xml_getName(i))
            || (std::string("h3") == xml_getName(i))
            || (std::string("h4") == xml_getName(i))
            || (std::string("h5") == xml_getName(i))
            || (std::string("h6") == xml_getName(i))
           )
       )
    {
      // these element start a phrasing context
      auto j = xml_getFirstChild(i);
      l.append(boxIt(i, j, rules, shape, l.getHeight(), layoutXML_Phrasing, xml_getPreviousSibling(i), X()));
      if (!xml_isEmpty(j))
      {
        throw XhtmlException_c("There was an unexpected tag within a phrasing context (" + getNodePath(i) + ")");
      }
      i = xml_getNextSibling(i);
    }
    else if (  (xml_isDataNode(i))
             ||(  (xml_isElementNode(i))
                &&(  (std::string("span") == xml_getName(i))
                   ||(std::string("b") == xml_getName(i))
                   ||(std::string("br") == xml_getName(i))
                   ||(std::string("code") == xml_getName(i))
                   ||(std::string("em") == xml_getName(i))
                   ||(std::string("q") == xml_getName(i))
                   ||(std::string("small") == xml_getName(i))
                   ||(std::string("strong") == xml_getName(i))
                   ||(std::string("sub") == xml_getName(i))
                   ||(std::string("sup") == xml_getName(i))
                   ||(std::string("img") == xml_getName(i))
                   ||(std::string("a") == xml_getName(i))
                  )
               )
            )
    {
      // these elements make the current node into a phrasing node
      // after parsing, we assume right now, i will be changed to point to the next node
      // not taken up by the Phrasing environment, so we don't want
      // i to be set to the next sibling as in all other cases
      l.append(layoutXML_Phrasing(i, rules, shape, l.getHeight()));
    }
    else if (xml_isElementNode(i) && std::string("table") == xml_getName(i))
    {
      l.append(boxIt(i, i, rules, shape, l.getHeight(), layoutXML_TABLE, xml_getPreviousSibling(i), X()));
      i = xml_getNextSibling(i);
    }
    else if (xml_isElementNode(i) && std::string("ul") == xml_getName(i))
    {
      l.append(boxIt(i, i, rules, shape, l.getHeight(), layoutXML_UL, xml_getPreviousSibling(i), X()));
      i = xml_getNextSibling(i);
    }
    else if (xml_isElementNode(i) && std::string("div") == xml_getName(i))
    {
      l.append(boxIt(i, i, rules, shape, l.getHeight(), layoutXML_Flow, xml_getPreviousSibling(i), X()));
      i = xml_getNextSibling(i);
    }
    else
    {
      throw XhtmlException_c("Only 'p', 'h1'-'h6', 'ul' and 'table' tag and prasing context is "
                             "is allowed within flow environment (" + getNodePath(i) + ")");
    }
  }

  l.setLeft(shape.getLeft(ystart, l.getHeight()));
  l.setRight(shape.getRight(ystart, l.getHeight()));

  return l;
}

template <class X>
TextLayout_c layoutXML_HTML(X & txt, const textStyleSheet_c & rules, const Shape_c & shape)
{
  TextLayout_c l;

  bool headfound = false;
  bool bodyfound = false;

  xml_forEachChild(txt, [&headfound, &bodyfound, &rules, &shape, &l](X i) -> bool {
    if (xml_isElementNode(i) && std::string("head") == xml_getName(i) && !headfound)
    {
      headfound = true;
    }
    else if (xml_isElementNode(i) && std::string("body") == xml_getName(i) && !bodyfound)
    {
      bodyfound = true;
      l = boxIt(i, i, rules, shape, 0, layoutXML_Flow, xml_getPreviousSibling(i), X());
    }
    else
    {
      throw XhtmlException_c("Only up to one 'head' and up to one 'body' tag and no other "
                             "tags are allowed inside the 'html' tag (" + getNodePath(i) + ")");
    }
    return false;
  });

  return l;
}



};



/** \brief layout the given preparsed XML tree as an HTML dom tree
 *  \param xml the xml tree to layout
 *  \param rules the stylesheet to use for layouting
 *  \param shape the shape to layout into
 */
template <class X>
TextLayout_c layoutXML(X txt, const textStyleSheet_c & rules, const Shape_c & shape)
{
  TextLayout_c l;

  if (!xml_isEmpty(txt))
  {

    if (!xml_isElementNode(txt) || std::string("html") != xml_getName(txt))
      throw XhtmlException_c("Top level tag must be the html tag (" + internal::getNodePath(txt) + ")");

    l = internal::layoutXML_HTML(txt, rules, shape);
  }

  return l;
}

/** \brief layout the given XHTML code
 *  \param lib the library to use, currently supported as Pugi and LibXML2
 *  \param txt the html text to parse, is must be utf-8. The text must be a proper XHTML document (see also \ref html_sec)
 *  \param rules the stylesheet to use for layouting
 *  \param shape the shape to layout into
 *  \attention it is not checked that txt is proper utf-8. If you have unsafe sources
 *  for your text to layout, use the check function from the utf-8 module
 */
#define layoutXHTML2(lib, txt) STLL::xml_parseString##lib(txt)
#define layoutXHTML(lib, txt, rules, shape) layoutXML(STLL::xml_getHeadNode(std::get<0>(layoutXHTML2(lib, txt))), rules, shape)

}

#endif
