#include "layouterXHTML.h"

#include "utf-8.h"

#include <vector>

#include <boost/lexical_cast.hpp>

namespace STLL {

class indentShape_c : public shape_c
{
  private:
    const shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    indentShape_c(const shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_right; }
};

class shiftShape_c : public shape_c
{
  private:
    const shape_c & outside;
    int32_t shift;

  public:
    shiftShape_c(const shape_c & s, int32_t sh) : outside(s), shift(sh) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getLeft(top+shift, bottom+shift); }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getRight(top+shift, bottom+shift); }
};

class stripLeftShape_c : public shape_c
{
  private:
    const shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    stripLeftShape_c(const shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getLeft(top, bottom)+ind_right; }
};

class stripRightShape_c : public shape_c
{
  private:
    const shape_c & outside;
    int32_t ind_left, ind_right;

  public:
    stripRightShape_c(const shape_c & s, int32_t li, int32_t ri) : outside(s), ind_left(li), ind_right(ri) { }

    virtual int32_t getLeft(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_left; }
    virtual int32_t getRight(int32_t top, int32_t bottom) const { return outside.getRight(top, bottom)-ind_right; }
};

static uint8_t hex2num(char c)
{
  switch (c)
  {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A': return 10;
    case 'b':
    case 'B': return 11;
    case 'c':
    case 'C': return 12;
    case 'd':
    case 'D': return 13;
    case 'e':
    case 'E': return 14;
    case 'f':
    case 'F': return 15;
    default:
      throw XhtmlException_c("Wrong format for a hex-number");
  }
}

static uint8_t hex2byte(char c1, char c2)
{
  return hex2num(c1)* 16 + hex2num(c2);
}

/** \brief evaluate a color string
 *  \param col string with the color
 *  \param r red value of the color
 *  \param g green value of the color
 *  \param b blue value of the color
 */
static void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b, uint8_t &a)
{
  if (col == "transparent")
  {
    r = g = b = a = 0;
    return;
  }

  if (col.length() != 7)
    throw XhtmlException_c("color string must be 7 characters long");

  if (col[0] == '#')
  {
    r = hex2byte(col[1], col[2]);
    g = hex2byte(col[3], col[4]);
    b = hex2byte(col[5], col[6]);
    a = 255;
  }
  else
    throw XhtmlException_c("only the # color scheme is supported and keyword transparent");
}

/** \brief evaluate size
 *  \param sz the size string from the CSS
 *  \return the resulting size in pixel
 */
static double evalSize(const std::string & sz)
{
  // right now we accept only pixel sizes
  size_t l = sz.length();

  if (sz[l-2] == 'p' && sz[l-1] == 'x')
  {
    return atof(sz.c_str());
  }

  throw XhtmlException_c("only pixel size format is supported");

  return 0;
}

static std::vector<codepointAttributes::shadow> evalShadows(const std::string & v)
{
  std::vector<codepointAttributes::shadow> s;

  codepointAttributes::shadow sh;

  size_t spos = 0;

  while (spos < v.length())
  {
    while (v[spos] == ' ' && spos < v.length()) spos++;
    if (spos >= v.length()) throw XhtmlException_c("Format of shadow invalid");

    sh.dx = evalSize(v.substr(spos, v.find(' ', spos)-spos));

    while (v[spos] != ' ' && spos < v.length()) spos++;
    if (spos >= v.length()) throw XhtmlException_c("Format of shadow invalid");

    while (v[spos] == ' ' && spos < v.length()) spos++;
    if (spos >= v.length()) throw XhtmlException_c("Format of shadow invalid");

    sh.dy = evalSize(v.substr(spos, v.find(' ', spos)-spos));

    while (v[spos] != ' ' && spos < v.length()) spos++;
    if (spos >= v.length()) throw XhtmlException_c("Format of shadow invalid");

    while (v[spos] == ' ' && spos < v.length()) spos++;
    if (spos >= v.length()) throw XhtmlException_c("Format of shadow invalid");

    evalColor(v.substr(spos, v.find(',', spos)-spos), sh.r, sh.g, sh.b, sh.a);

    s.push_back(sh);

    while (v[spos] != ',' && spos < v.length()) spos++;
    if (spos >= v.length()) break;

    spos++;
  }

  return s;
}

static std::string normalizeHTML(const std::string & in, char prev)
{
  std::string out;

  for (auto a : in)
  {
    if (a == '\n' || a == '\r')
      a = ' ';

    if (a != ' ' || prev != ' ')
      out += a;

    prev = a;
  }

  return out;
}

static std::string getNodePath(const pugi::xml_node & xml)
{
  if (xml.empty())
    return "";
  else
    return getNodePath(xml.parent()) + "/" + xml.name();
}

static std::shared_ptr<fontFace_c> getFontForNode(const pugi::xml_node & xml, const textStyleSheet_c & rules)
{
  std::string fontFamily = rules.getValue(xml, "font-family");
  std::string fontStyle = rules.getValue(xml, "font-style");
  std::string fontVariant = rules.getValue(xml, "font-variant");
  std::string fontWeight = rules.getValue(xml, "font-weight");
  double fontSize = evalSize(rules.getValue(xml, "font-size"));

  auto f = rules.findFamily(fontFamily)->getFont(64*fontSize, fontStyle, fontVariant, fontWeight);

  if (!f)
  {
    throw XhtmlException_c(std::string("Requested font not found (family:'") + fontFamily +
                                       "', style: '" + fontStyle +
                                       "', variant: '" + fontVariant +
                                       "', weight: '" + fontWeight + ") required here: " + getNodePath(xml));
  }

  return f;
}

static const pugi::char_t * getHTMLAttribute(pugi::xml_node xml, const std::string attr)
{
  while (true)
  {
    const pugi::char_t * res = xml.attribute(attr.c_str()).value();

    if (res && *res) return res;

    if ("lang" == attr)
    {
      xml = xml.parent();

      if (xml.empty())
      {
        return res;
      }
    }
    else
    {
      return res;
    }
  }
}

static void layoutXML_text(const pugi::xml_node & xml, const textStyleSheet_c & rules, std::u32string & txt,
               attributeIndex_c & attr)
{
  for (const auto & i : xml)
  {
    if (i.type() == pugi::node_pcdata)
    {
      size_t s = txt.length();

      if (txt.length() == 0)
        txt = u8_convertToU32(normalizeHTML(i.value(), ' '));
      else
        txt += u8_convertToU32(normalizeHTML(i.value(), txt[txt.length()-1]));

      codepointAttributes a;

      evalColor(rules.getValue(xml, "color"), a.r, a.g, a.b, a.a);
      a.font = getFontForNode(xml, rules);
      a.lang = getHTMLAttribute(xml, "lang");
      a.flags = 0;
      if (rules.getValue(xml, "text-decoration") == "underline")
      {
        a.flags |= codepointAttributes::FL_UNDERLINE;
      }
      a.shadows = evalShadows(rules.getValue(xml, "text-shadow"));

      attr.set(s, txt.length(), a);
    }
    else if (   (i.type() == pugi::node_element)
             && (   (std::string("i") == i.name())
                 || (std::string("div") == i.name())
                )
            )
    {
      layoutXML_text(i, rules, txt, attr);
    }
    else if ((i.type() == pugi::node_element) && (std::string("br") == i.name()))
    {
      txt += U'\n';
      codepointAttributes a;
      a.flags = 0;
      a.font = getFontForNode(xml, rules);
      a.lang = getHTMLAttribute(xml, "lang");
      attr.set(txt.length()-1, a);
    }
    else
    {
      throw XhtmlException_c("Within paragraph environments only text and 'i' and 'div' "
                             "tags are allowed (" + getNodePath(i) + ")");
    }
  }
}

typedef textLayout_c (*ParseFunction)(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart);


// handles padding, margin and border, all in one, it takes the text returned from the
// ParseFunction and boxes it
static textLayout_c boxIt(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart, ParseFunction fkt)
{
  int32_t padding = evalSize(rules.getValue(xml, "padding"));
  int32_t borderwidth = evalSize(rules.getValue(xml, "border-width"));
  int32_t margin = evalSize(rules.getValue(xml, "margin"));
  int32_t marginElementAbove = 0;
  if (!xml.previous_sibling().empty())
  {
    marginElementAbove = evalSize(rules.getValue(xml.previous_sibling(), "margin"));
  }

  int32_t topmargin = std::max(marginElementAbove, margin)-marginElementAbove;

  auto l2 = fkt(xml, rules,
                indentShape_c(shape, padding+borderwidth+margin, padding+borderwidth+margin),
                ystart+padding+borderwidth+topmargin);
  l2.setHeight(l2.getHeight()+padding+borderwidth+margin);

  textLayout_c::commandData c;
  if (borderwidth)
  {
    c.command = textLayout_c::commandData::CMD_RECT;

    std::string color = rules.getValue(xml, "border-color");
    if (color == "")
      color = rules.getValue(xml, "color");
    evalColor(color, c.r, c.g, c.b, c.a);

    if (c.a != 0)
    {
      c.x = shape.getLeft(ystart+topmargin, ystart+topmargin)+margin;
      c.y = ystart+topmargin;
      c.w = shape.getRight(ystart+topmargin, ystart+topmargin)-shape.getLeft(ystart+topmargin, ystart+topmargin)-2*margin;
      c.h = borderwidth;
      l2.addCommandStart(c);

      c.y = l2.getHeight()-borderwidth-margin;
      l2.addCommandStart(c);

      c.x = shape.getRight(ystart+topmargin, ystart+topmargin)-borderwidth-margin;
      c.y = ystart+topmargin;
      c.w = borderwidth;
      c.h = l2.getHeight()-ystart-margin-topmargin;
      l2.addCommandStart(c);

      c.x = shape.getLeft(ystart+topmargin, ystart+topmargin)+margin;
      l2.addCommandStart(c);
    }
  }

  evalColor(rules.getValue(xml, "background-color"), c.r, c.g, c.b, c.a);

  if (c.a != 0)
  {
    c.command = textLayout_c::commandData::CMD_RECT;

    c.x = shape.getLeft(ystart+topmargin, ystart+topmargin)+borderwidth+margin;
    c.y = ystart+borderwidth+topmargin;
    c.w = shape.getRight(ystart+topmargin, ystart+topmargin)-
          shape.getLeft(ystart+topmargin, ystart+topmargin)-2*borderwidth-2*margin;
    c.h = l2.getHeight()-ystart-2*borderwidth-margin-topmargin;
    l2.addCommandStart(c);
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

  return l2;
}

// this whole stuff is a recursive descending parser of the XHTML stuff
static textLayout_c layoutXML_P(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart)
{
  std::u32string txt;
  attributeIndex_c attr;

  layoutXML_text(xml, rules, txt, attr);

  layoutProperties lprop;
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

  return layoutParagraph(txt, attr, shape, lprop, ystart);
}

static textLayout_c layoutXML_UL(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart)
{
  textLayout_c l;
  l.setHeight(ystart);
  for (const auto & i : xml)
  {
    if (   (i.type() == pugi::node_element)
        && (std::string("li") == i.name())
       )
    {
      pugi::xml_node j = xml;
      while (j.type() != pugi::node_pcdata)
        j = j.first_child();

      auto font = getFontForNode(j, rules);
      auto y = l.getHeight();

      codepointAttributes a;
      evalColor(rules.getValue(xml, "color"), a.r, a.g, a.b, a.a);
      a.font = font;
      a.lang = "";
      a.flags = 0;

      int32_t padding = evalSize(rules.getValue(i, "padding"));
      int32_t listIndent = font->getAscender()/64;

      auto direction = rules.getValue(xml, "direction");

      layoutProperties prop;
      prop.indent = 0;
      prop.ltr = true;

      if (direction == "ltr")
      {
        prop.align = layoutProperties::ALG_CENTER;
        l.append(layoutParagraph(U"\u2022", attributeIndex_c(a),
                                 stripLeftShape_c(shape, padding, padding+listIndent), prop, y+padding));
        l.append(boxIt(i, rules, indentShape_c(shape, listIndent, 0), y, layoutXML_P));
      }
      else
      {
        prop.align = layoutProperties::ALG_CENTER;
        l.append(layoutParagraph(U"\u2022", attributeIndex_c(a),
                                 stripRightShape_c(shape, padding+listIndent, padding), prop, y+padding));
        l.append(boxIt(i, rules, indentShape_c(shape, 0, listIndent), y, layoutXML_P));
      }
    }
    else
    {
      throw XhtmlException_c("Only 'li' tags allowed within 'ul' tag (" + getNodePath(i) + ")");
    }
  }

  return l;
}

static textLayout_c layoutXML_BODY(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart)
{
  textLayout_c l;
  l.setHeight(ystart);

  for (const auto & i : txt)
  {
    if (   (i.type() == pugi::node_element)
        && (   (std::string("p") == i.name())
            || (std::string("h1") == i.name())
            || (std::string("h2") == i.name())
            || (std::string("h3") == i.name())
            || (std::string("h4") == i.name())
            || (std::string("h5") == i.name())
            || (std::string("h6") == i.name())
           )
       )
    {
      l.append(boxIt(i, rules, shape, l.getHeight(), layoutXML_P));
    }
    else if (i.type() == pugi::node_element && std::string("table") == i.name())
    {
    }
    else if (i.type() == pugi::node_element && std::string("ul") == i.name())
    {
      l.append(boxIt(i, rules, shape, l.getHeight(), layoutXML_UL));
    }
    else
    {
      throw XhtmlException_c("Only 'p', 'h1'-'h6', 'ul' and 'table' tag is allowed within a "
                             "body tag (" + getNodePath(i) + ")");
    }
  }

  return l;
}

static textLayout_c layoutXML_HTML(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  textLayout_c l;

  bool headfound = false;
  bool bodyfound = false;

  for (const auto & i : txt)
  {
    if (i.type() == pugi::node_element && std::string("head") == i.name() && !headfound)
    {
      headfound = true;
    }
    else if (i.type() == pugi::node_element && std::string("body") == i.name() && !bodyfound)
    {
      bodyfound = true;
      l = boxIt(i, rules, shape, 0, layoutXML_BODY);
    }
    else
    {
      throw XhtmlException_c("Only up to one 'head' and up to one 'body' tag and no other "
                             "tags are allowed inside the 'html' tag (" + getNodePath(i) + ")");
    }
  }

  return l;
}

textLayout_c layoutXML(const pugi::xml_document & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  textLayout_c l;

  // we must have a HTML root node
  for (const auto & i : txt)
  {
    if (i.type() == pugi::node_element && std::string("html") == i.name())
    {
      l = layoutXML_HTML(i, rules, shape);
    }
    else
    {
      throw XhtmlException_c("Top level tag must be the html tag (" + getNodePath(i) + ")");
    }
  }

  return l;
}

textLayout_c layoutXHTML(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  pugi::xml_document doc;

  auto res = doc.load_buffer(txt.c_str(), txt.length());

  if (res)
    return layoutXML(doc, rules, shape);
  else
  {
    throw XhtmlException_c(std::string("Error Parsing XHTML [") + doc.child("node").attribute("attr").value() + "]\n" +
            "Error description: " + res.description() + "\n" +
            "Error offset: " + boost::lexical_cast<std::string>(res.offset) + "  " +
            txt.substr(std::max<int>(res.offset-20, 0), 20) + "[here]" + txt.substr(res.offset, 20));
  }
}

}
