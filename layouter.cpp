#include "layouter.h"

#include "utf-8.h"

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <fribidi/fribidi.h>

#include <linebreak.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

typedef struct
{
  std::vector<textLayout_c::commandData> run;
  int dx, dy;
  FriBidiLevel embeddingLevel;
  char linebreak;
  std::shared_ptr<fontFace_c> font;
} runInfo;

// TODO create a sharing structure, where we only
// define structures for each configuration once
// and link to it
// but for now it is good as it is
typedef struct
{
  uint8_t r, g, b;
  std::shared_ptr<fontFace_c> font;
  std::string lang;
} codepointAttributes;

FriBidiLevel getBidiEmbeddingLevels(const std::u32string & txt32, std::vector<FriBidiLevel> & embedding_levels)
{
  std::vector<FriBidiCharType> bidiTypes(txt32.length());
  fribidi_get_bidi_types((uint32_t*)txt32.c_str(), txt32.length(), bidiTypes.data());
  FriBidiParType base_dir = FRIBIDI_TYPE_LTR_VAL; // TODO depends on main script of text
  embedding_levels.resize(txt32.length());
  FriBidiLevel max_level = fribidi_get_par_embedding_levels(bidiTypes.data(), txt32.length(),
                                                            &base_dir, embedding_levels.data());

  return max_level;
}

textLayout_c layoutParagraph(const std::u32string & txt32, const std::vector<codepointAttributes> & attr, const shape_c & shape)
{
  // calculate embedding types for the text
  std::vector<FriBidiLevel> embedding_levels;
  FriBidiLevel max_level = getBidiEmbeddingLevels(txt32, embedding_levels);

  // calculate the possible linebreak positions
  std::vector<char> linebreaks(txt32.length());
  set_linebreaks_utf32((utf32_t*)txt32.c_str(), txt32.length(), "", linebreaks.data());

  // Get our harfbuzz font structs, TODO we need to do that for all the fonts
  std::map<const std::shared_ptr<fontFace_c>, hb_font_t *> hb_ft_fonts;

  for (const auto & a : attr)
  {
    if (hb_ft_fonts.find(a.font) == hb_ft_fonts.end())
    {
      hb_ft_fonts[a.font] = hb_ft_font_create(a.font->getFace(), NULL);
    }
  }

  // Create a buffer for harfbuzz to use
  hb_buffer_t *buf = hb_buffer_create();

  std::string lan = attr[0].lang.substr(0, 2);
  std::string s = attr[0].lang.substr(3, 4);

  hb_script_t scr = hb_script_from_iso15924_tag(HB_TAG(s[0], s[1], s[2], s[3]));
  hb_buffer_set_script(buf, scr);
  // TODO must come either from text or from rules
  hb_buffer_set_language(buf, hb_language_from_string(lan.c_str(), lan.length()));

  size_t runstart = 0;

  std::vector<runInfo> runs;

  while (runstart < txt32.length())
  {
    size_t spos = runstart+1;
    // find end of current run
    while (   (spos < txt32.length())
           && (embedding_levels[runstart] == embedding_levels[spos])
           && (attr[runstart].font == attr[spos].font)
           && (   (linebreaks[spos-1] == LINEBREAK_NOBREAK)
               || (linebreaks[spos-1] == LINEBREAK_INSIDEACHAR)
              )
          )
    {
      spos++;
    }

    hb_buffer_add_utf32(buf, ((uint32_t*)txt32.c_str())+runstart, spos-runstart, 0, spos-runstart);

    if (embedding_levels[runstart] % 2 == 0)
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    }
    else
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    }

    hb_shape(hb_ft_fonts[attr[runstart].font], buf, NULL, 0);

    unsigned int         glyph_count;
    hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

    runInfo run;

    run.dx = run.dy = 0;
    run.embeddingLevel = embedding_levels[runstart];
    run.linebreak = linebreaks[spos-1];
    run.font = attr[runstart].font;

    for (size_t j=0; j < glyph_count; ++j)
    {
      textLayout_c::commandData g;

      g.glyphIndex = glyph_info[j].codepoint;
      g.font = attr[runstart].font;
      // TODO the other parameters

      g.x = run.dx + (glyph_pos[j].x_offset/64);
      g.y = run.dy - (glyph_pos[j].y_offset/64);

      run.dx += glyph_pos[j].x_advance/64;
      run.dy -= glyph_pos[j].y_advance/64;

      g.r = attr[glyph_info[j].cluster + runstart].r;
      g.g = attr[glyph_info[j].cluster + runstart].g;
      g.b = attr[glyph_info[j].cluster + runstart].b;

      g.command = textLayout_c::commandData::CMD_GLYPH;

      run.run.push_back(g);
    }

    runs.push_back(run);
    runstart = spos;

    hb_buffer_reset(buf);
  }

  hb_buffer_destroy(buf);

  for (auto & a : hb_ft_fonts)
    hb_font_destroy(a.second);

  std::vector<size_t> runorder(runs.size());
  int n(0);
  std::generate(runorder.begin(), runorder.end(), [&]{ return n++; });

  // layout a run
  // TODO take care of different font sizes of the different runs
  runstart = 0;
  int32_t ypos = runs[0].font->getAscender()/64;
  textLayout_c l;

  while (runstart < runs.size())
  {
    // todo use actual font line height
    int32_t left = shape.getLeft(ypos-runs[runstart].font->getAscender()/64, ypos-runs[runstart].font->getDescender()/64) + runs[runstart].dx;
    int32_t right = shape.getRight(ypos-runs[runstart].font->getAscender()/64, ypos-runs[runstart].font->getDescender()/64);

    size_t spos = runstart + 1;

    while (spos < runs.size() && left+runs[spos].dx < right)
    {
      left += runs[spos].dx;
      spos++;
    }

    // reorder runs for current line
    for (int i = max_level-1; i >= 0; i--)
    {
      // find starts of regions to reverse
      for (size_t j = runstart; j < spos; j++)
      {
        if (runs[runorder[j]].embeddingLevel > i)
        {
          // find the end of the current regions
          size_t k = j+1;
          while (k < spos && runs[runorder[k]].embeddingLevel > i)
          {
            k++;
          }

          std::reverse(runorder.begin()+j, runorder.begin()+k);
          j = k;
        }
      }
    }

    int32_t xpos = shape.getLeft(ypos-runs[runstart].font->getAscender(), ypos-runs[runstart].font->getDescender());

    for (size_t i = runstart; i < spos; i++)
    {
      l.addCommandVector(runs[runorder[i]].run, xpos, ypos);
      xpos += runs[runorder[i]].dx;
    }

    ypos += runs[runstart].font->getHeigt()/64;
    runstart = spos;
  }


  return l;
}

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
    default:  return 0;
  }
}

static uint8_t hex2byte(char c1, char c2)
{
  return hex2num(c1)* 16 + hex2num(c2);

}

static void evalColor(const std::string & col, uint8_t & r, uint8_t & g, uint8_t &b)
{
  if (col[0] == '#' && col.length() >= 7)
  {
    r = hex2byte(col[1], col[2]);
    g = hex2byte(col[3], col[4]);
    b = hex2byte(col[5], col[6]);
  }
}

static double evalSize(const std::string & sz)
{
  // right now we accept only pixel sizes
  size_t l = sz.length();

  if (sz[l-2] == 'p' && sz[l-1] == 'x')
  {
    return atof(sz.c_str());
  }

  // TODO exception
  return 0;
}

void layoutXML_text(const pugi::xml_node & xml, const textStyleSheet_c & rules, std::u32string & txt,
               std::vector<codepointAttributes> & attr)
{
  for (const auto & i : xml)
  {
    if (i.type() == pugi::node_pcdata)
    {
      txt += u8_convertToU32(i.value());

      codepointAttributes a;

      evalColor(rules.getValue(xml, "color"), a.r, a.g, a.b);
      std::string fontFamily = rules.getValue(xml, "font-family");
      std::string fontStyle = rules.getValue(xml, "font-style");
      double fontSize = evalSize(rules.getValue(xml, "font-size"));

      a.font = rules.findFamily(fontFamily)->getFont(64*fontSize, fontStyle);
      a.lang = "en-eng";

      while (attr.size() < txt.length())
        attr.push_back(a);
    }
    else if (i.type() == pugi::node_element && std::string("i") == i.name())
    {
      layoutXML_text(i, rules, txt, attr);
    }
  }
}

// this whole stuff is a recursive descending parser of the XHTML stuff
textLayout_c layoutXML_P(const pugi::xml_node & xml, const textStyleSheet_c & rules, const shape_c & shape)
{
  std::u32string txt;
  std::vector<codepointAttributes> attr;

  layoutXML_text(xml, rules, txt, attr);

  return layoutParagraph(txt, attr, shape);
}

textLayout_c layoutXML_BODY(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  textLayout_c l;

  for (const auto & i : txt)
  {
    if (i.type() == pugi::node_element && std::string("p") == i.name())
    {
      l.append(layoutXML_P(i, rules, shape));
    }
    else if (i.type() == pugi::node_element && std::string("table") == i.name())
    {
    }
    else
    {
      // TODO exception nothing else supported
    }
  }

  return l;
}

textLayout_c layoutXML_HTML(const pugi::xml_node & txt, const textStyleSheet_c & rules, const shape_c & shape)
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
  // TODO handle parser errors
  doc.load_buffer(txt.c_str(), txt.length());
  return layoutXML(doc, rules, shape);
}

textLayout_c layoutRaw(const std::string & txt, const std::shared_ptr<fontFace_c> font, const shape_c & shape, const std::string & language)
{
  // when we layout raw text we
  // only have to convert the text to utf-32
  // and assign the given font and language to all the codepoints of that text

  std::u32string txt32 = u8_convertToU32(txt);
  std::vector<codepointAttributes> attr(txt32.size());

  for (auto & i : attr)
  {
    i.r = i.g = i.b = 255;
    i.font = font;
    i.lang = language;
  }

  return layoutParagraph(txt32, attr, shape);
}

static bool ruleFits(const std::string & sel, const pugi::xml_node & node)
{
  if (sel == "*")         return true;
  if (sel == node.name()) return true;

  return false;
}

static uint16_t rulePrio(const std::string & sel)
{
  if (sel == "*") return 0;

  return 1;
}

const std::string & textStyleSheet_c::getValue(const pugi::xml_node & node, const std::string & attribute) const
{
  static std::string defaultValue("");

  // go through all rules, check only the ones that give a value to the requested attribute
  // evaluate rule by priority (look at the CSS priority rules
  // choose the highest priority
  int16_t bestPrio = -1;
  std::string & res = defaultValue;


  for (auto & r : rules)
  {
    if (ruleFits(r.selector, node) && r.attribute == attribute && rulePrio(r.selector) > bestPrio)
    {
      res = r.value;
      bestPrio = rulePrio(r.selector);
    }
  }

  return res;
}

void textStyleSheet_c::font(const std::string& family, const std::string& file, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  if (families.size())
  {
    font(families.begin()->second->getCache(), family, file, style, variant, weight, stretch);
  }
  else
  {
    font(std::make_shared<fontCache_c>(), family, file, style, variant, weight, stretch);
  }
}

void textStyleSheet_c::font(std::shared_ptr<fontCache_c> fc, const std::string& family, const std::string& file, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  auto i = families.find(family);

  if (i == families.end())
  {
    i = families.insert(std::make_pair(family, std::make_shared<fontFamily_c>(fc))).first;
  }

  i->second->addFont(file, style, variant, weight, stretch);
}

