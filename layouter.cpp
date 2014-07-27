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
} runInfo;

textLayout_c layout(const std::string & txt, const textStyleSheet_c & rules, const shape_c & shape)
{
  /* Get our harfbuzz font structs */
  std::shared_ptr<fontFace_c> font = rules.findFamily("sans")->getFont(50*64);
  hb_font_t *hb_ft_font = hb_ft_font_create(font->getFace(), NULL);

  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();

  std::u32string txt32 = u8_convertToU32(txt);

  std::vector<FriBidiCharType> bidiTypes(txt32.length());
  fribidi_get_bidi_types((uint32_t*)txt32.c_str(), txt32.length(), bidiTypes.data());
  std::vector<FriBidiLevel> embedding_levels(txt32.length());
  FriBidiParType base_dir = FRIBIDI_TYPE_LTR_VAL; // TODO depends on main script of text
  FriBidiLevel max_level = fribidi_get_par_embedding_levels(bidiTypes.data(), txt32.length(),
                                                            &base_dir, embedding_levels.data());

  std::vector<char> linebreaks(txt32.length());
  set_linebreaks_utf32((utf32_t*)txt32.c_str(), txt32.length(), "", linebreaks.data());

  std::string lan = rules.language.substr(0, 2);
  std::string s = rules.language.substr(3, 4);

  hb_script_t scr = hb_script_from_iso15924_tag(HB_TAG(s[0], s[1], s[2], s[3]));

  size_t runstart = 0;

  std::vector<runInfo> runs;

  while (runstart < txt32.length())
  {
    size_t spos = runstart+1;
    // find end of current run
    while (   (spos < txt32.length())
           && (embedding_levels[runstart] == embedding_levels[spos])
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

    // TODO must come either from text or from rules
    hb_buffer_set_script(buf, scr);
    hb_buffer_set_language(buf, hb_language_from_string(lan.c_str(), lan.length()));

    hb_shape(hb_ft_font, buf, NULL, 0);

    unsigned int         glyph_count;
    hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

    runInfo run;

    run.dx = run.dy = 0;
    run.embeddingLevel = embedding_levels[runstart];
    run.linebreak = linebreaks[spos-1];

    for (size_t j=0; j < glyph_count; ++j)
    {
      textLayout_c::commandData g;

      g.glyphIndex = glyph_info[j].codepoint;
      g.font = font;
      // TODO the other parameters

      g.x = run.dx + (glyph_pos[j].x_offset/64);
      g.y = run.dy - (glyph_pos[j].y_offset/64);

      run.dx += glyph_pos[j].x_advance/64;
      run.dy -= glyph_pos[j].y_advance/64;

      g.r = g.g = g.b = 255;

      g.command = textLayout_c::commandData::CMD_GLYPH;

      run.run.push_back(g);
    }

    runs.push_back(run);
    runstart = spos;

    hb_buffer_reset(buf);
  }

  hb_buffer_destroy(buf);
  hb_font_destroy(hb_ft_font);

  std::vector<size_t> runorder(runs.size());
  int n(0);
  std::generate(runorder.begin(), runorder.end(), [&]{ return n++; });

  // layout a run
  runstart = 0;
  int32_t ypos = 0;
  textLayout_c l;

  while (runstart < runs.size())
  {
    // todo use actual font line height
    int32_t left = shape.getLeft(ypos, ypos) + runs[runstart].dx;
    int32_t right = shape.getRight(ypos, ypos);

    int spos = runstart + 1;

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

    int32_t xpos = shape.getLeft(ypos, ypos);

    for (size_t i = runstart; i < spos; i++)
    {
      l.addCommandVector(runs[runorder[i]].run, xpos, ypos);
      xpos += runs[runorder[i]].dx;
    }

    runstart = spos;
    ypos += font->getHeigt()/64;
  }


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

