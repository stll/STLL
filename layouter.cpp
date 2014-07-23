#include "layouter.h"

#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <string>
#include <memory>


textLayout_c layout(const std::string & txt, const textStyleSheet_c & rules)
{
  /* Get our harfbuzz font structs */
  std::shared_ptr<fontFace_c> font = rules.findFamily("sans")->getFont(50*64);
  hb_font_t *hb_ft_font = hb_ft_font_create(font->getFace(), NULL);

  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();

  std::string lan = rules.language.substr(0, 2);
  std::string s = rules.language.substr(3, 4);

  hb_script_t scr = hb_script_from_iso15924_tag(HB_TAG(s[0], s[1], s[2], s[3]));

  // TODO must come either from text or from rules
  hb_buffer_set_direction(buf, hb_script_get_horizontal_direction(scr)); // make harfbuzz take this information from script
  hb_buffer_set_script(buf, scr);
  hb_buffer_set_language(buf, hb_language_from_string(lan.c_str(), lan.length()));

  /* Layout the text */
  hb_buffer_add_utf8(buf, txt.c_str(), txt.length(), 0, txt.length());
  hb_shape(hb_ft_font, buf, NULL, 0);

  unsigned int         glyph_count;
  hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

  int x = 0;
  int y = 0;

  textLayout_c l;

  for (unsigned j=0; j < glyph_count; ++j)
  {
    textLayout_c::commandData g;

    g.glyphIndex = glyph_info[j].codepoint;
    g.font = font;
    // TODO the other parameters

    int gx = x + (glyph_pos[j].x_offset/64);
    int gy = y - (glyph_pos[j].y_offset/64);

    g.x = gx;
    g.y = gy;

    x += glyph_pos[j].x_advance/64;
    y -= glyph_pos[j].y_advance/64;

    g.r = g.g = g.b = 255;

    g.command = textLayout_c::commandData::CMD_GLYPH;

    l.addCommand(g);
  }

  hb_buffer_destroy(buf);
  hb_font_destroy(hb_ft_font);

  return l;
}

const std::string & textStyleSheet_c::getValue(const pugi::xml_node & node, const std::string & attribute) const
{
  // go through all rules, check only the ones that give a value to the requested attribute
  // evaluate rule by priority (look at the CSS priority rules
  // choose the highest priority
  return language;
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

