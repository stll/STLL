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
#include "layouterFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_LCD_FILTER_H

#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <string>
#include <memory>

namespace STLL {

// TODO the fontFace_c constructor and library interface is not perfect...

FontFace_c::FontFace_c(std::shared_ptr<FreeTypeLibrary_c> l, const FontFileResource_c & r, uint32_t s) :
                lib(l), rec(r), size(s)
{
  f = lib->newFace(r, s);
}

FontFace_c::~FontFace_c()
{
  lib->doneFace(f);
}

uint32_t FontFace_c::getHeight(void) const
{
  return f->size->metrics.height;
}

int32_t FontFace_c::getAscender(void) const
{
  return f->size->metrics.ascender;
}

int32_t FontFace_c::getDescender(void) const
{
  return f->size->metrics.descender;
}

int32_t FontFace_c::getUnderlinePosition(void) const
{
  return static_cast<int64_t>(f->underline_position*f->size->metrics.y_scale) / 65536;
}

int32_t FontFace_c::getUnderlineThickness(void) const
{
  return static_cast<int64_t>(f->underline_thickness*f->size->metrics.y_scale) / 65536;
}

std::shared_ptr<FontFace_c> FontCache_c::getFont(const FontFileResource_c & res, uint32_t size)
{
  FontFaceParameter_c ffp(res, size);

  auto i = fonts.find(ffp);

  if (i != fonts.end())
  {
    auto a = i->second;

    if (a) return a;
  }

  // TODO... race maybe someone else opens a font here?

  auto a = std::make_shared<FontFace_c>(lib, res, size);

  fonts.insert(std::make_pair(ffp, a));

  return a;
}


Font_c FontCache_c::getFont(const FontResource_c & res, uint32_t size)
{
  Font_c f;

  for (auto & r : res)
  {
    f.add(getFont(r, size));
  }

  return f;
}

FT_GlyphSlotRec_ * FontFace_c::renderGlyph(glyphIndex_t glyphIndex, SubPixelArrangement sp)
{
  FT_Render_Mode rm;

  switch(sp)
  {
    case SUBP_RGB:
    case SUBP_BGR:
      rm = FT_RENDER_MODE_LCD;
      break;

    case SUBP_RGB_V:
    case SUBP_BGR_V:
      rm = FT_RENDER_MODE_LCD_V;
      break;

    default:
      rm = FT_RENDER_MODE_NORMAL;
      break;
  }

  /* load glyph image into the slot (erase previous one) */
  if (FT_Load_Glyph(f, glyphIndex, FT_LOAD_TARGET_LIGHT)) return 0;
  if (FT_Render_Glyph(f->glyph, rm)) return 0;

  return f->glyph;
}

bool FontFace_c::containsGlyph(char32_t ch)
{
  return FT_Get_Char_Index(f, ch) != 0;
}

FT_Face FreeTypeLibrary_c::newFace(const FontFileResource_c & r, uint32_t size)
{
  FT_Face f;
  FT_Open_Args a;
  if (r.getDatasize() == 0) {
      a.flags = FT_OPEN_PATHNAME;
      a.pathname = const_cast<FT_String*>(r.getDescription().c_str());  // TODO const correctness is not given
      a.num_params = 0;
      a.params = nullptr;
  } else {
      a.flags = FT_OPEN_MEMORY;
      a.memory_base = r.getData().get();
      a.memory_size = r.getDatasize();
      a.num_params = 0;
      a.params = nullptr;
  }
  if (FT_Open_Face(lib, &a, 0, &f))
  {
    throw FreetypeException_c(std::string("Could not open Font '") + r.getDescription() + "' maybe "
                              "file is spelled wrong or file is broken");
  }

  if (FT_Set_Pixel_Sizes(f, (size+32)/64, (size+32)/64))
  {
    doneFace(f);

    throw FreetypeException_c(std::string("Could not set the requested file to font '") +
                              r.getDescription() + "'");
  }

  /*  See http://www.microsoft.com/typography/otspec/name.htm
   *        for a list of some possible platform-encoding pairs.
   *        We're interested in 0-3 aka 3-1 - UCS-2.
   *        Otherwise, fail. If a font has some unicode map, but lacks
   *        UCS-2 - it is a broken or irrelevant font. What exactly
   *        Freetype will select on face load (it promises most wide
   *        unicode, and if that will be slower that UCS-2 - left as
   *        an excercise to check. */
  for(int i = 0; i < f->num_charmaps; i++)
  {
    if (   (   (f->charmaps[i]->platform_id == 0)
            && (f->charmaps[i]->encoding_id == 3))
        || (   (f->charmaps[i]->platform_id == 3)
            && (f->charmaps[i]->encoding_id == 1))
       )
    {
      if (FT_Set_Charmap(f, f->charmaps[i]))
      {
        doneFace(f);
        throw FreetypeException_c(std::string("Could not set a unicode character map to font '") +
                                  r.getDescription() + "'. Maybe the font doesn't have one?");
      }
      return f;
    }
  }

  doneFace(f);
  throw FreetypeException_c(std::string("Could not find a unicode character map to font '") +
                            r.getDescription() + "'. Maybe the font doesn't have one?");
}

void FreeTypeLibrary_c::doneFace(FT_Face f)
{
  FT_Done_Face(f);
}

FreeTypeLibrary_c::~FreeTypeLibrary_c()
{
  FT_Done_FreeType(lib);
}

FreeTypeLibrary_c::FreeTypeLibrary_c()
{
  if (FT_Init_FreeType(&lib))
  {
    throw FreetypeException_c("Could not initialize font rendering library instance");
  }

  FT_Library_SetLcdFilter(lib, FT_LCD_FILTER_DEFAULT);
}

std::shared_ptr<FontFace_c> Font_c::get(char32_t codepoint) const
{
  for (auto f : fonts)
    if (f->containsGlyph(codepoint)) return f;

  if (fonts.size())
    return fonts[0];

  return 0;
}


void FontFamily_c::addFont(const FontResource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  FontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  fonts[par] = res;
}

Font_c FontFamily_c::getFont(uint32_t size, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  FontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  auto i = fonts.find(par);

  // when the font is not found, return an empty pointer
  if (i == fonts.end())
  {
    return Font_c();
  }
  else
  {
    return cache->getFont(i->second, size);
  }
}

}
