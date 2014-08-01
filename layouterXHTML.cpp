#include "layouterXHTML.h"

#include "utf-8.h"

#include <vector>

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

static std::shared_ptr<fontFace_c> getFontForNode(pugi::xml_node xml, const textStyleSheet_c & rules)
{
  std::string fontFamily = rules.getValue(xml, "font-family");
  std::string fontStyle = rules.getValue(xml, "font-style");
  std::string fontVariant = rules.getValue(xml, "font-variant");
  std::string fontWeight = rules.getValue(xml, "font-weight");
  double fontSize = evalSize(rules.getValue(xml, "font-size"));

  return rules.findFamily(fontFamily)->getFont(64*fontSize, fontStyle, fontVariant, fontWeight);
}

static void layoutXML_text(pugi::xml_node xml, const textStyleSheet_c & rules, std::u32string & txt,
               std::vector<codepointAttributes> & attr)
{
  for (const auto & i : xml)
  {
    if (i.type() == pugi::node_pcdata)
    {
      if (txt.length() == 0)
        txt = u8_convertToU32(normalizeHTML(i.value(), ' '));
      else
        txt += u8_convertToU32(normalizeHTML(i.value(), txt[txt.length()-1]));

      codepointAttributes a;

      evalColor(rules.getValue(xml, "color"), a.r, a.g, a.b);
      a.font = getFontForNode(xml, rules);
      a.lang = "en-eng";

      while (attr.size() < txt.length())
        attr.push_back(a);
    }
    else if (   (i.type() == pugi::node_element)
             && (   (std::string("i") == i.name())
                 || (std::string("div") == i.name())
                )
            )
    {
      layoutXML_text(i, rules, txt, attr);
    }
  }
}

// this whole stuff is a recursive descending parser of the XHTML stuff
static textLayout_c layoutXML_P(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape)
{
  std::u32string txt;
  std::vector<codepointAttributes> attr;

  layoutXML_text(xml, rules, txt, attr);

  return layoutParagraph(txt, attr, shape, rules.getValue(xml, "text-align"));
}

static textLayout_c layoutXML_UL(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  textLayout_c l;
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
      l.append(layoutRaw(u8"\u2022", font, shape, "en-eng"), 0, y);
      l.append(layoutXML_P(i, rules, indentShape_c(shape, font->getAscender()/64, 0)), 0, y);
    }
    else
    {
      // TODO esception?
    }
  }

  return l;
}

static textLayout_c layoutXML_BODY(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  textLayout_c l;

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
      l.append(layoutXML_P(i, rules, shape), 0, l.getHeight());
    }
    else if (i.type() == pugi::node_element && std::string("table") == i.name())
    {
    }
    else if (i.type() == pugi::node_element && std::string("ul") == i.name())
    {
      l.append(layoutXML_UL(i, rules, shape), 0, l.getHeight());
    }
    else
    {
      // TODO exception nothing else supported
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
    if (std::string("head") == i.name() && !headfound)
    {
      headfound = true;
    }
    else if (std::string("body") == i.name() && !bodyfound)
    {
      bodyfound = true;
      l = layoutXML_BODY(i, rules, shape);
    }
    else
    {
      // nothing else permitted -> exception TODO
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
    if (std::string("html") == i.name())
    {
      l = layoutXML_HTML(i, rules, shape);
    }
    else
    {
      // nothing else permitted -> exception TODO
    }
  }

  return l;
}

textLayout_c layoutXHTML(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  pugi::xml_document doc;
  // TODO preprocess to get rid of linebreaks and multiple spaces


  // TODO handle parser errors
  doc.load_buffer(txt.c_str(), txt.length());
  return layoutXML(doc, rules, shape);
}
