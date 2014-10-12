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

#include "layouter.h"

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <fribidi/fribidi.h>

#include <linebreak.h>

#include <algorithm>

namespace STLL {

// TODO better error checking, throw our own exceptions, e.g. when a link was not properly
// specified

typedef struct
{
  // the commands to output this run
  std::vector<std::pair<size_t, textLayout_c::commandData>> run;

  // the advance information of this run
  int dx, dy;

  // the embedding level (text direction) of this run
  FriBidiLevel embeddingLevel;

  // linebreak information for AFTER this run, for values see liblinebreak
  char linebreak;

  // the font used for this run... will probably be identical to
  // the fonts in the run
  std::shared_ptr<fontFace_c> font;

  // is this run a space run? Will be removed at line ends
  bool space;

  // is this a soft hypen?? will ony be shown at line ends
  bool shy;

  // ascender and descender of this run
  int32_t ascender, descender;

  // link boxes for this run
  std::vector<textLayout_c::linkInformation> links;

#ifndef NDEBUG
  // the text of this run, useful for debugging to see what is going on
  std::u32string text;
#endif

} runInfo;


// create the text direction information using libfribidi
static FriBidiLevel getBidiEmbeddingLevels(const std::u32string & txt32,
                                           std::vector<FriBidiLevel> & embedding_levels,
                                           FriBidiParType base_dir)
{
  std::vector<FriBidiCharType> bidiTypes(txt32.length());
  fribidi_get_bidi_types(reinterpret_cast<const uint32_t*>(txt32.c_str()), txt32.length(), bidiTypes.data());
  embedding_levels.resize(txt32.length());
  FriBidiLevel max_level = fribidi_get_par_embedding_levels(bidiTypes.data(), txt32.length(),
                                                            &base_dir, embedding_levels.data());

  return max_level;
}

static int32_t roundToDivisible(int32_t r, int32_t d)
{
  if (d > 0 && d < 64)
  {
    r = ( d*r + 32 ) / 64;
    r = (64*r + d/2) / d;
  }

  return r;
}

static bool isBidiCharacter(char32_t c)
{
  if (c == U'\U0000202A' || c == U'\U0000202B' || c == U'\U0000202C')
    return true;
  else
    return false;
}

// use harfbuzz to layout runs of text
static std::vector<runInfo> createTextRuns(const std::u32string & txt32,
                                           const attributeIndex_c & attr,
                                           const std::vector<FriBidiLevel> & embedding_levels,
                                           const std::vector<char> & linebreaks,
                                           const layoutProperties & prop
                                          )
{
  // Get our harfbuzz font structs
  std::map<const std::shared_ptr<fontFace_c>, hb_font_t *> hb_ft_fonts;

  // also get the maximal shadow numbers, so that we know how many layers there are
  size_t normalLayer = 0;

  for (size_t i = 0; i < txt32.length(); i++)
  {
    if (!isBidiCharacter(txt32[i]))
    {
      auto a = attr.get(i);

      if (a.font && (hb_ft_fonts.find(a.font) == hb_ft_fonts.end()))
      {
        hb_ft_fonts[a.font] = hb_ft_font_create(a.font->getFace(), NULL);
      }

      normalLayer = std::max(normalLayer, attr.get(i).shadows.size());
    }
  }

  // Create a buffer for harfbuzz to use
  hb_buffer_t *buf = hb_buffer_create();

  size_t runstart = 0;

  while (runstart < txt32.length() && isBidiCharacter(txt32[runstart])) runstart++;

  std::vector<runInfo> runs;

  while (runstart < txt32.length())
  {
    size_t spos = runstart+1;
    // find end of current run
    while (   (spos < txt32.length())
           && (   isBidiCharacter(txt32[spos])
               || (  (embedding_levels[runstart] == embedding_levels[spos])
                  && (attr.get(runstart).lang == attr.get(spos).lang)
                  && (attr.get(runstart).font == attr.get(spos).font)
                  && (attr.get(runstart).baseline_shift == attr.get(spos).baseline_shift)
                  && (!attr.get(spos).inlay)
                  && (!attr.get(spos-1).inlay)
                  && (   (linebreaks[spos-1] == LINEBREAK_NOBREAK)
                      || (linebreaks[spos-1] == LINEBREAK_INSIDEACHAR)
                     )
                  && (txt32[spos] != U' ')
                  && (txt32[spos-1] != U' ')
                  && (txt32[spos] != U'\n')
                  && (txt32[spos-1] != U'\n')
                  && (txt32[spos] != U'\u00AD')
                  )
               )
          )
    {
      spos++;
    }

    runInfo run;

    if (txt32[spos-1] == U' ' || txt32[spos-1] == U'\n')
    {
      run.space = true;
    }
    else
    {
      run.space = false;
    }

    run.shy = txt32[runstart] == U'\u00AD';

    std::string language = attr.get(runstart).lang;

    // reset is not required, when no language is set, the buffer
    // reset automatically resets the language and scribt info as well
    if (language != "")
    {
      size_t i = language.find_first_of('-');

      if (i != std::string::npos)
      {
        hb_script_t scr = hb_script_from_iso15924_tag(HB_TAG(language[i+1], language[i+2],
                                                             language[i+3], language[i+4]));
        hb_buffer_set_script(buf, scr);

        hb_buffer_set_language(buf, hb_language_from_string(language.c_str(), i-1));
      }
      else
      {
        hb_buffer_set_language(buf, hb_language_from_string(language.c_str(), language.length()));
      }
    }

    if (!run.shy)
      hb_buffer_add_utf32(buf, reinterpret_cast<const uint32_t*>(txt32.c_str())+runstart, spos-runstart, 0, spos-runstart);
    else
      hb_buffer_add_utf32(buf, reinterpret_cast<const uint32_t*>(U"\u2010"), 1, 0, 1);

    if (embedding_levels[runstart] % 2 == 0)
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    }
    else
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    }

    if (attr.get(runstart).font)
      hb_shape(hb_ft_fonts[attr.get(runstart).font], buf, NULL, 0);

    unsigned int         glyph_count;
    hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

    run.dx = run.dy = 0;
    run.embeddingLevel = embedding_levels[runstart];
    run.linebreak = linebreaks[spos-1];
    run.font = attr.get(runstart).font;
    if (attr.get(runstart).inlay)
    {
      run.ascender = attr.get(runstart).inlay->getHeight()+attr.get(runstart).baseline_shift;
      run.descender = attr.get(runstart).inlay->getHeight()-run.ascender;
    }
    else
    {
      run.ascender = run.font->getAscender()+attr.get(runstart).baseline_shift;
      run.descender = run.font->getDescender()+attr.get(runstart).baseline_shift;
    }
#ifndef NDEBUG
    run.text = txt32.substr(runstart, spos-runstart);
#endif

    size_t curLink = 0;
    textLayout_c::rectangle_c linkRect;
    int linkStart;

    for (size_t j=0; j < glyph_count; ++j)
    {
      if (isBidiCharacter(txt32[glyph_info[j].cluster + runstart]))
        continue;

      auto a = attr.get(glyph_info[j].cluster + runstart);

      if (!curLink && a.link || curLink != a.link)
      {
        linkStart = run.dx;
      }

      if (a.inlay)
      {
        for (auto in : a.inlay->getData())
        {
          // if ascender is 0 we want to be below the baseline
          // but if we leave the inlay where is is the top line of them
          // image will be _on_ the baseline, which is not what we want
          // so we actually need to go one below
          in.y -= (run.ascender-1);
          in.x += run.dx;

          run.run.push_back(std::make_pair(normalLayer, in));
        }

        if (a.flags & codepointAttributes::FL_UNDERLINE)
        {
          textLayout_c::commandData g;

          g.command = textLayout_c::commandData::CMD_RECT;
          g.x = run.dx;
          g.w = a.inlay->getRight();

          if (prop.underlineFont)
          {
            g.y = -((prop.underlineFont->getUnderlinePosition()+prop.underlineFont->getUnderlineThickness()/2));
            g.h = std::max(64, prop.underlineFont->getUnderlineThickness());
          }
          else
          {
            g.y = -((a.font->getUnderlinePosition()+a.font->getUnderlineThickness()/2));
            g.h = std::max(64, a.font->getUnderlineThickness());
          }

          for (size_t j = 0; j < a.shadows.size(); j++)
          {
            g.x += a.shadows[j].dx;
            g.y += a.shadows[j].dy;

            g.c = a.shadows[j].c;

            run.run.push_back(std::make_pair(j, g));

            g.x -= a.shadows[j].dx;
            g.y -= a.shadows[j].dy;
          }

          g.c = a.c;

          run.run.push_back(std::make_pair(normalLayer, g));
        }

        run.dx += a.inlay->getRight();
      }
      else
      {
        textLayout_c::commandData g;

        g.command = textLayout_c::commandData::CMD_GLYPH;

        g.glyphIndex = glyph_info[j].codepoint;
        g.font = attr.get(runstart).font;

        g.x = run.dx + (glyph_pos[j].x_offset);
        g.y = run.dy - (glyph_pos[j].y_offset)-attr.get(runstart).baseline_shift;

        g.x = roundToDivisible(g.x, prop.round);
        g.y = roundToDivisible(g.y, prop.round);

        for (size_t j = 0; j < attr.get(runstart).shadows.size(); j++)
        {
          g.x += attr.get(runstart).shadows[j].dx;
          g.y += attr.get(runstart).shadows[j].dy;

          g.c = attr.get(runstart).shadows[j].c;

          run.run.push_back(std::make_pair(j, g));

          g.x -= attr.get(runstart).shadows[j].dx;
          g.y -= attr.get(runstart).shadows[j].dy;
        }

        run.dx += glyph_pos[j].x_advance;
        run.dy -= glyph_pos[j].y_advance;

        run.dx = roundToDivisible(run.dx, prop.round);
        run.dy = roundToDivisible(run.dy, prop.round);

        g.c = a.c;

        run.run.push_back(std::make_pair(normalLayer, g));

        if (a.flags & codepointAttributes::FL_UNDERLINE)
        {
          g.command = textLayout_c::commandData::CMD_RECT;
          g.w = glyph_pos[j].x_advance+64;

          if (prop.underlineFont)
          {
            g.h = std::max(64, prop.underlineFont->getUnderlineThickness());
            g.y = -((prop.underlineFont->getUnderlinePosition()+prop.underlineFont->getUnderlineThickness()/2));
          }
          else
          {
            g.h = std::max(64, a.font->getUnderlineThickness());
            g.y = -((a.font->getUnderlinePosition()+a.font->getUnderlineThickness()/2));
          }

          for (size_t j = 0; j < attr.get(runstart).shadows.size(); j++)
          {
            g.x += attr.get(runstart).shadows[j].dx;
            g.y += attr.get(runstart).shadows[j].dy;

            g.c = attr.get(runstart).shadows[j].c;

            run.run.push_back(std::make_pair(j, g));

            g.x -= attr.get(runstart).shadows[j].dx;
            g.y -= attr.get(runstart).shadows[j].dy;
          }

          g.command = textLayout_c::commandData::CMD_RECT;
          g.c = a.c;
          run.run.push_back(std::make_pair(normalLayer, g));
        }
      }

      if (a.link)
      {
        if (curLink && curLink != a.link)
        {
          textLayout_c::linkInformation l;
          l.url = prop.links[curLink-1];
          l.areas.push_back(linkRect);
          run.links.push_back(l);
          curLink = 0;
        }

        if (!curLink)
        {
          linkRect.x = linkStart;
          linkRect.y = -run.ascender;
          linkRect.w = run.dx-linkStart;
          linkRect.h = run.ascender-run.descender;
          curLink = a.link;
        }
        else
        {
          linkRect.w = run.dx-linkStart;
        }
      }
    }

    // finalize an open link
    if (curLink)
    {
      textLayout_c::linkInformation l;
      l.url = prop.links[curLink-1];
      l.areas.push_back(linkRect);
      run.links.push_back(l);
      curLink = 0;
    }

    runs.push_back(run);
    runstart = spos;

    while (runstart < txt32.length() && isBidiCharacter(txt32[runstart])) runstart++;

    hb_buffer_reset(buf);
  }

  hb_buffer_destroy(buf);

  for (auto & a : hb_ft_fonts)
    hb_font_destroy(a.second);

  return runs;
}

static void mergeLinks(textLayout_c & txt, const std::vector<textLayout_c::linkInformation> & links, int dx, int dy)
{
  for (const auto & l : links)
  {

    // try to find the link to insert in the already existing links within txt
    auto i = std::find_if(txt.links.begin(), txt.links.end(),
                          [&l] (textLayout_c::linkInformation l2) { return l.url == l2.url; }
                         );

    // when not found create it
    if (i == txt.links.end())
    {
      textLayout_c::linkInformation l2;
      l2.url = l.url;
      txt.links.emplace_back(l2);
      i = txt.links.end()-1;
    }

    // copy over the rectangles from the link and offset them
    for (auto r : l.areas)
    {
      r.x += dx;
      r.y += dy;

      i->areas.push_back(r);
    }
  }
}

static textLayout_c breakLines(std::vector<runInfo> & runs,
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
  enum { FL_FIRST, FL_BREAK, FL_NORMAL } firstline = FL_FIRST;
  bool forcebreak = false;

  // while there are runs left to do
  while (runstart < runs.size())
  {
    // accumulate enough runs to fill the line, this is done by accumulating runs
    // until we come to a place where we might break the line
    // then we check if the line would be too long with the new set of runs

    // skip initial spaces
    while (runstart < runs.size() && runs[runstart].space) runstart++;

    // these variables contain the current line information
    // which run it starts at, when the first run that will go onto the next
    // line, how many spaces there are in the line (for justification)
    // and what the width of the runs is
    int32_t curAscend = 0;
    int32_t curDescend = 0;
    int32_t curWidth = 0;
    size_t spos = runstart;
    size_t numSpace = 0;
    forcebreak = false;

    // if it is a first line, we add the indent first
    if ((firstline != FL_NORMAL) && prop.align != layoutProperties::ALG_CENTER) curWidth = prop.indent;

    // now go through the remaining runs and add them
    while (spos < runs.size())
    {
      // calculate the line information including the added runs
      // we start with the current line settings
      int32_t newAscend = curAscend;
      int32_t newDescend = curDescend;
      int32_t newWidth = curWidth;
      size_t newspos = spos;
      size_t newSpace = numSpace;

      // now add runs, until we get to a new point where we can break
      // the line, or we run out of runs
      while (newspos < runs.size())
      {
        // update line hight and with with the new run
        newAscend = std::max(newAscend, runs[newspos].ascender);
        newDescend = std::min(newDescend, runs[newspos].descender);
        newWidth += runs[newspos].dx;
        if (runs[newspos].space) newSpace++;

        // if we come to a point where we can break the line, we stop and
        // evaluate if these new added runs still fits
        // we break out here, if the next run is a space and will allow breaking
        // after the space (this will remove spaces at the end of the line
        // we also break out, if we can break after the current run
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
        {
          break;
        }

        // next run
        newspos++;
      }

      // we have included runs up to newspos, we will continue with the one after that
      // so increment once more
      newspos++;

      // check, if the line still fits in the available space
      // if not break out and don't take over the new additional
      // but even if it doesn't fit, we need to take over when we have
      // not yet anything in our line, this might happen when there is one
      // run that is longer than the available space
      if (   (spos > runstart)
          && (shape.getLeft(ypos, ypos+newAscend-newDescend)+newWidth >
              shape.getRight(ypos, ypos+newAscend-newDescend))
         )
      {
        // next run would overrun
        break;
      }

      // when the final character at the end of the line (prior to the latest
      // additions is a shy, remove that one from the width, because them
      // shy will only be output at the end of the line
      if ((spos > runstart) && runs[spos-1].shy) newWidth -= runs[spos-1].dx;

      // additional run fits, so take over the new line
      curAscend = newAscend;
      curDescend = newDescend;
      curWidth = newWidth;
      spos = newspos;
      numSpace = newSpace;

      // the current end of the line forces a break, or the next character is a space and forces a break
      if (  (runs[spos-1].linebreak == LINEBREAK_MUSTBREAK)
          ||((spos < runs.size()) && runs[spos].space && runs[spos].linebreak == LINEBREAK_MUSTBREAK)
         )
      {
        forcebreak = true;
        break;
      }
    }

    // this normally doesn't happen because the final run will never be a space
    // because the linebreak will then happen before that space, but just in case
    // remove it from the space counter
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

    // calculate how many pixels are left on the line (for justification)
    int32_t spaceLeft = shape.getRight(ypos, ypos+curAscend-curDescend) -
                        shape.getLeft(ypos, ypos+curAscend-curDescend) - curWidth;

    // depending on the paragraph alignment settings we calculate where at the
    // x-axis we start with the runs and how many additional pixels we add to a space
    int32_t xpos;
    double spaceadder = 0;

    switch (prop.align)
    {
      default:
      case layoutProperties::ALG_LEFT:
        xpos = shape.getLeft(ypos, ypos+curAscend-curDescend);
        if (firstline != FL_NORMAL) xpos += prop.indent;
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
        if (numSpace > 1 && spos < runs.size() && !forcebreak)
          spaceadder = 1.0 * spaceLeft / numSpace;

        if (firstline != FL_NORMAL) xpos += prop.indent;

        break;

      case layoutProperties::ALG_JUSTIFY_RIGHT:
        // don't justify last paragraph
        if (numSpace > 1 && spos < runs.size() && !forcebreak)
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

    // go down to the baseline
    ypos += curAscend;

    // find the number of layers that we need to output
    size_t maxlayer = 0;
    for (size_t i = runstart; i < spos; i++)
      for (auto & r : runs[runorder[i]].run)
        maxlayer = std::max(maxlayer, r.first+1);

    for (uint32_t layer = 0; layer < maxlayer; layer++)
    {
      int32_t xpos2 = xpos;
      numSpace = 0;

      // output runs of current layer
      for (size_t i = runstart; i < spos; i++)
      {
        if (!runs[runorder[i]].shy || i+1 == spos)
        {
          // output only non-space runs
          if (!runs[runorder[i]].space)
          {
            for (auto & cc : runs[runorder[i]].run)
            {
              if (cc.first == layer)
              {
                cc.second.x += xpos2+spaceadder*numSpace;
                cc.second.y += ypos;
                l.addCommand(cc.second);
              }
            }
          }
          else
          {
            // in space runs, there may be an rectangular command that represents
            // the underline, make that underline longer by spaceadder
            for (auto & cc : runs[runorder[i]].run)
            {
              if (   (cc.first == layer)
                  && (cc.second.command == textLayout_c::commandData::CMD_RECT)
                 )
              {
                cc.second.w += spaceadder;
                cc.second.x += xpos2+spaceadder*numSpace;
                cc.second.y += ypos;
                l.addCommand(cc.second);
              }
            }

            // the link rectangle in spaces also needs to get longer
            if (!runs[runorder[i]].links.empty() && !runs[runorder[i]].links[0].areas.empty())
            {
              runs[runorder[i]].links[0].areas[0].w += spaceadder;
            }
          }

          if (layer == 0)
            mergeLinks(l, runs[runorder[i]].links, xpos2+spaceadder*numSpace, ypos);

          // count the spaces
          if (runs[runorder[i]].space) numSpace++;

          // advance the x-position
          xpos2 += runs[runorder[i]].dx;
        }
      }
    }

    if (firstline == FL_FIRST)
    {
      l.setFirstBaseline(ypos);
    }

    // go the the top of the next line
    ypos -= curDescend;

    // set the runstart at the next run and skip space runs
    runstart = spos;

    // if we have a forced break, the next line will be like the first
    // in the way that is will be indented
    if (forcebreak)
      firstline = FL_BREAK;
    else
      firstline = FL_NORMAL;
  }

  l.setHeight(ypos);
  l.setLeft(shape.getLeft2(ystart, ypos));
  l.setRight(shape.getRight2(ystart, ypos));

  return l;
}

static std::vector<char> getLinebreaks(const std::u32string & txt32, const attributeIndex_c & attr)
{
  size_t length = txt32.length();

  std::vector<char> linebreaks(length);

  size_t runstart = 0;

  while (runstart < length && isBidiCharacter(txt32[runstart])) runstart++;

  while (runstart < length)
  {
    size_t runpos = runstart+1;

    while (runpos < length && (isBidiCharacter(txt32[runpos]) || attr.get(runstart).lang == attr.get(runpos).lang))
    {
      runpos++;
    }

    // when calculating the length for the function call below, we need to keep in mind
    // that the function will always force a linebreak at the end of the string, to avoid
    // this when the string really goes on, we include the next character in the string
    // to linebreak (except of course when the string really ends here), that way we get
    // a real linebreak and the wrongly written break is overwritten in the next call
    set_linebreaks_utf32(reinterpret_cast<const utf32_t*>(txt32.c_str())+runstart,
                         runpos-runstart+(runpos < length ? 1 : 0),
                         attr.get(runstart).lang.c_str(), linebreaks.data()+runstart);

    runstart = runpos;
    while ((runstart < length) && isBidiCharacter(txt32[runstart])) runstart++;
  }

  return linebreaks;
}

textLayout_c layoutParagraph(const std::u32string & txt32, const attributeIndex_c & attr,
                             const shape_c & shape, const layoutProperties & prop, int32_t ystart)
{
  // calculate embedding types for the text
  std::vector<FriBidiLevel> embedding_levels;
  FriBidiLevel max_level = getBidiEmbeddingLevels(txt32, embedding_levels,
                                prop.ltr ? FRIBIDI_TYPE_LTR_VAL : FRIBIDI_TYPE_RTL_VAL);

  // calculate the possible linebreak positions
  auto linebreaks = getLinebreaks(txt32, attr);

  // create runs of layout text. Each run is a cohesive set, e.g. a word with a single
  // font, ...
  std::vector<runInfo> runs = createTextRuns(txt32, attr, embedding_levels, linebreaks, prop);

  // layout the runs into lines
  return breakLines(runs, shape, max_level, prop, ystart);
}

}
