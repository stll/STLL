#include "layouterXHTML.h"

#include "utf-8.h"

#include <vector>

#include <boost/lexical_cast.hpp>

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

      evalColor(rules.getValue(xml, "color"), a.r, a.g, a.b);
      a.font = getFontForNode(xml, rules);
      a.lang = "en-eng";

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
      a.font = getFontForNode(xml, rules);
      // TODO language
      attr.set(txt.length()-1, a);
    }
    else
    {
      throw XhtmlException_c("Within paragraph environments only text and 'i' and 'div' "
                             "tags are allowed (" + getNodePath(i) + ")");
    }
  }
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
    else if (s == "")      lprop.align = layoutProperties::ALG_JUSTIFY_LEFT;
    else
    {
      throw XhtmlException_c("Only 'left' and 'right' are allowed as values for the "
                             "'text-align-last' CSS property (" + getNodePath(xml) + ")");
    }
  }
  else
  {
    throw XhtmlException_c("Only 'left, 'right', 'center' and 'justify' are allowed for "
                           "the 'text-align' CSS property (" + getNodePath(xml) + ")");
  }

  lprop.indent = evalSize(rules.getValue(xml, "text-indent"));

  int32_t padding = evalSize(rules.getValue(xml, "padding"));

  auto l = layoutParagraph(txt, attr, indentShape_c(shape, padding, padding), lprop, ystart+padding);

  l.setHeight(padding + l.getHeight());

  return l;
}

static textLayout_c layoutXML_UL(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape, int32_t ystart)
{
  textLayout_c l;
  l.setHeight(ystart);
  for (const auto & i : txt)
  {
    if (   (i.type() == pugi::node_element)
        && (std::string("li") == i.name())
       )
    {
      // TODO depends on direction of text
      pugi::xml_node j = txt;
      while (j.type() != pugi::node_pcdata)
        j = j.first_child();

      auto font = getFontForNode(j, rules);
      auto y = l.getHeight();

      // TODO better indentation, todo colour of bullet right now fixed to white
      codepointAttributes a;
      evalColor(rules.getValue(txt, "color"), a.r, a.g, a.b);
      a.font = font;
      int32_t padding = evalSize(rules.getValue(i, "padding"));

      // TODO do properly
      a.lang = "en-engl";

      layoutProperties prop;
      prop.align = layoutProperties::ALG_LEFT;
      prop.indent = 0;

      l.append(layoutParagraph(U"\u2022", attributeIndex_c(a), indentShape_c(shape, padding, padding), prop, y+padding));
      l.append(layoutXML_P(i, rules, indentShape_c(shape, font->getAscender()/64, 0), y));

      l.setHeight(l.getHeight()+padding);
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
      // TODO rahmen und anderes beachten
      l.append(layoutXML_P(i, rules, shape, l.getHeight()));
    }
    else if (i.type() == pugi::node_element && std::string("table") == i.name())
    {
    }
    else if (i.type() == pugi::node_element && std::string("ul") == i.name())
    {
      l.append(layoutXML_UL(i, rules, shape, l.getHeight()));
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
      int32_t padding = evalSize(rules.getValue(i, "padding"));
      l = layoutXML_BODY(i, rules, indentShape_c(shape, padding, padding), 10);
      l.setHeight(l.getHeight()+padding);
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
  // TODO preprocess to get rid of linebreaks and multiple spaces


  // TODO handle parser errors
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
