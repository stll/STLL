#include "layouter.h"

#include "utf-8.h"

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <fribidi/fribidi.h>

#include <linebreak.h>

#include <algorithm>

#define _DEBUG_

typedef struct
{
  std::vector<textLayout_c::commandData> run;
  int dx, dy;
  FriBidiLevel embeddingLevel;
  char linebreak;
  std::shared_ptr<fontFace_c> font;
  bool space;   // is this run a space run?
  bool shy;  // is this a soft hypen??
#ifdef _DEBUG_
  std::u32string text;
#endif
} runInfo;


static FriBidiLevel getBidiEmbeddingLevels(const std::u32string & txt32, std::vector<FriBidiLevel> & embedding_levels)
{
  std::vector<FriBidiCharType> bidiTypes(txt32.length());
  fribidi_get_bidi_types((uint32_t*)txt32.c_str(), txt32.length(), bidiTypes.data());
  FriBidiParType base_dir = FRIBIDI_TYPE_LTR_VAL; // TODO depends on main script of text
  embedding_levels.resize(txt32.length());
  FriBidiLevel max_level = fribidi_get_par_embedding_levels(bidiTypes.data(), txt32.length(),
                                                            &base_dir, embedding_levels.data());

  return max_level;
}

static std::vector<runInfo> createTextRuns(const std::u32string & txt32,
                                           const attributeIndex_c & attr,
                                           const std::vector<FriBidiLevel> & embedding_levels,
                                           const std::vector<char> & linebreaks
                                          )
{
  // Get our harfbuzz font structs
  std::map<const std::shared_ptr<fontFace_c>, hb_font_t *> hb_ft_fonts;

  for (size_t i = 0; i < txt32.length(); i++)
  {
    auto a = attr.get(i);

    if (hb_ft_fonts.find(a.font) == hb_ft_fonts.end())
    {
      hb_ft_fonts[a.font] = hb_ft_font_create(a.font->getFace(), NULL);
    }
  }

  // Create a buffer for harfbuzz to use
  hb_buffer_t *buf = hb_buffer_create();

  // TODO language information should go to harfbuzz and liblinebreak for each section
  std::string lan = attr.get(0).lang.substr(0, 2);
  std::string s = attr.get(0).lang.substr(3, 4);

//  hb_script_t scr = hb_script_from_iso15924_tag(HB_TAG(s[0], s[1], s[2], s[3]));
//  hb_buffer_set_script(buf, scr);
  // TODO must come either from text or from rules
//  hb_buffer_set_language(buf, hb_language_from_string(lan.c_str(), lan.length()));

  size_t runstart = 0;

  std::vector<runInfo> runs;

  while (runstart < txt32.length())
  {
    size_t spos = runstart+1;
    // find end of current run
    while (   (spos < txt32.length())
           && (embedding_levels[runstart] == embedding_levels[spos])
           && (attr.get(runstart).font == attr.get(spos).font)
           && (   (linebreaks[spos-1] == LINEBREAK_NOBREAK)
               || (linebreaks[spos-1] == LINEBREAK_INSIDEACHAR)
              )
           && (txt32[spos] != U' ')
           && (txt32[spos-1] != U' ')
           && (txt32[spos] != U'\u00AD')
          )
    {
      spos++;
    }

    runInfo run;

    if (txt32[spos-1] == U' ')
    {
      run.space = true;
    }
    else
    {
      run.space = false;
    }

    run.shy = txt32[runstart] == U'\u00AD';

    if (!run.shy)
      hb_buffer_add_utf32(buf, ((uint32_t*)txt32.c_str())+runstart, spos-runstart, 0, spos-runstart);
    else
      hb_buffer_add_utf32(buf, (uint32_t*)U"\u2010", 1, 0, 1);

    if (embedding_levels[runstart] % 2 == 0)
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    }
    else
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    }

    hb_shape(hb_ft_fonts[attr.get(runstart).font], buf, NULL, 0);

    unsigned int         glyph_count;
    hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

    run.dx = run.dy = 0;
    run.embeddingLevel = embedding_levels[runstart];
    run.linebreak = linebreaks[spos-1];
    run.font = attr.get(runstart).font;
#ifdef _DEBUG_
    run.text = txt32.substr(runstart, spos-runstart);
#endif

    for (size_t j=0; j < glyph_count; ++j)
    {
      textLayout_c::commandData g;

      g.glyphIndex = glyph_info[j].codepoint;
      g.font = attr.get(runstart).font;
      // TODO the other parameters

      g.x = run.dx + (glyph_pos[j].x_offset/64);
      g.y = run.dy - (glyph_pos[j].y_offset/64);

      run.dx += glyph_pos[j].x_advance/64;
      run.dy -= glyph_pos[j].y_advance/64;

      g.r = attr.get(glyph_info[j].cluster + runstart).r;
      g.g = attr.get(glyph_info[j].cluster + runstart).g;
      g.b = attr.get(glyph_info[j].cluster + runstart).b;

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

  return runs;
}

static textLayout_c breakLines(const std::vector<runInfo> & runs,
                               const shape_c & shape,
                               FriBidiLevel max_level,
                               const layoutProperties & prop, int32_t ystart)
{
  std::vector<size_t> runorder(runs.size());
  int n(0);
  std::generate(runorder.begin(), runorder.end(), [&]{ return n++; });

  // layout a paragraph line by line
  size_t runstart = 0;
  int32_t ypos = ystart;
  textLayout_c l;

  while (runstart < runs.size() && runs[runstart].space) runstart++;

  while (runstart < runs.size())
  {
    int32_t curAscend = 0;
    int32_t curDescend = 0;
    int32_t curWidth = 0;
    size_t spos = runstart;
    size_t numSpace = 0;

    // add runs until we run out of runs
    while (spos < runs.size())
    {
      // check, if we can add another run
      // TODO take properly care of spaces at the end of lines (they must be left out)

      int32_t newAscend = curAscend;
      int32_t newDescend = curDescend;
      int32_t newWidth = curWidth;
      size_t newspos = spos;
      size_t newSpace = numSpace;

      while (newspos < runs.size())
      {
        newAscend = std::max(newAscend, runs[newspos].font->getAscender()/64);
        newDescend = std::min(newDescend, runs[newspos].font->getDescender()/64);
        newWidth += runs[newspos].dx;
        if (runs[newspos].space) newSpace++;
        if (  (    (newspos+1) < runs.size()
                && (runs[newspos+1].space)
                && (   (runs[newspos+1].linebreak == LINEBREAK_ALLOWBREAK)
                    || (runs[newspos+1].linebreak == LINEBREAK_MUSTBREAK))
              )
            ||(    (!runs[newspos].space)
                && (   (runs[newspos].linebreak == LINEBREAK_ALLOWBREAK)
                    || (runs[newspos].linebreak == LINEBREAK_MUSTBREAK))
              )
           )
          break;
        newspos++;
      }

      newspos++;

      if (   (spos > runstart)
          && (shape.getLeft(ypos, ypos+newAscend-newDescend)+newWidth >
              shape.getRight(ypos, ypos+newAscend-newDescend))
         )
      {
        // next run would overrun
        break;
      }

      if ((spos > runstart) && runs[spos-1].shy) newWidth -= runs[spos-1].dx;

      // additional run fits
      curAscend = newAscend;
      curDescend = newDescend;
      curWidth = newWidth;
      spos = newspos;
      numSpace = newSpace;
    }

    if (runs[spos-1].space) numSpace--;

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

    int32_t spaceLeft = shape.getRight(ypos, ypos+curAscend-curDescend) -
                        shape.getLeft(ypos, ypos+curAscend-curDescend);
    spaceLeft -= curWidth;

    int32_t xpos;
    double spaceadder = 0;

    switch (prop.align)
    {
      case layoutProperties::ALG_LEFT:
        xpos = shape.getLeft(ypos, ypos+curAscend-curDescend);
        break;

      case layoutProperties::ALG_RIGHT:
        xpos = shape.getLeft(ypos, ypos+curAscend-curDescend) + spaceLeft;
        break;

      case layoutProperties::ALG_CENTER:
        xpos = shape.getLeft(ypos, ypos+curAscend-curDescend) + spaceLeft/2;
        break;

      case layoutProperties::ALG_JUSTIFY_LEFT:
        xpos = shape.getLeft(ypos, ypos+curAscend-curDescend);
        // don't justify last paragraph
        if (numSpace > 1 && spos < runs.size())
          spaceadder = 1.0 * spaceLeft / numSpace;
        break;

      case layoutProperties::ALG_JUSTIFY_RIGHT:
        // don't justify last paragraph
        if (numSpace > 1 && spos < runs.size())
        {
          xpos = shape.getLeft(ypos, ypos+curAscend-curDescend);
          spaceadder = 1.0 * spaceLeft / numSpace;
        }
        else
        {
          xpos = shape.getLeft(ypos, ypos+curAscend-curDescend) + spaceLeft;
        }
        break;
    }

    ypos += curAscend;
    numSpace = 0;

    for (size_t i = runstart; i < spos; i++)
    {
      if (!runs[i].shy || i+1 == spos)
      {
        l.addCommandVector(runs[runorder[i]].run, xpos+spaceadder*numSpace, ypos);
        if (runs[runorder[i]].space) numSpace++;
        xpos += runs[runorder[i]].dx;
      }
    }

    ypos -= curDescend;
    runstart = spos;
    while (runstart < runs.size() && runs[runstart].space) runstart++;
  }

  // TODO proper font handling for multiple fonts in a line
  l.setHeight(ypos);

  return l;
}

textLayout_c layoutParagraph(const std::u32string & txt32, const attributeIndex_c & attr,
                             const shape_c & shape, const layoutProperties & prop, int32_t ystart)
{
  // calculate embedding types for the text
  std::vector<FriBidiLevel> embedding_levels;
  FriBidiLevel max_level = getBidiEmbeddingLevels(txt32, embedding_levels);

  // calculate the possible linebreak positions
  std::vector<char> linebreaks(txt32.length());
  set_linebreaks_utf32((utf32_t*)txt32.c_str(), txt32.length(), "", linebreaks.data());

  std::vector<runInfo> runs = createTextRuns(txt32, attr, embedding_levels, linebreaks);

  return breakLines(runs, shape, max_level, prop, ystart);
}

textLayout_c layoutRaw(const std::string & txt, codepointAttributes a,
                       const shape_c & shape, int32_t ystart)
{
  // when we layout raw text we
  // only have to convert the text to utf-32
  // and assign the given font and language to all the codepoints of that text

  std::u32string txt32 = u8_convertToU32(txt);
  attributeIndex_c attr;

  attr.set(0, a);

  layoutProperties prop;
  prop.align = layoutProperties::ALG_LEFT;

  return layoutParagraph(txt32, attr, shape, prop, ystart);
}
