#include "layouterFont.h"

#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <string>
#include <memory>

namespace STLL {

// TODO the fontFace_c constructor and library interface is not perfect...

fontFace_c::fontFace_c(std::shared_ptr<freeTypeLibrary_c> l, const fontRessource_c & res, uint32_t size) : lib(l)
{
  f = lib->newFace(res, size);
}

fontFace_c::~fontFace_c()
{
  lib->doneFace(f);
}

std::shared_ptr<fontFace_c> fontCache_c::getFont(const fontRessource_c & res, uint32_t size)
{
  fontFaceParameter_c ffp(res, size);

  auto i = fonts.find(ffp);

  if (i != fonts.end())
  {
    auto a = i->second.lock();

    if (a)
      return a;
  }

  // TODO... race maybe someone else opens a font here?

  auto f = std::make_shared<fontFace_c>(lib, res, size);

  fonts[ffp] = f;

  return f;
}

void fontFace_c::outlineRender(uint32_t idx, FT_Raster_Params* params)
{
  if (FT_Load_Glyph(f, idx, FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT))
  {
    throw FreetypeException_c("A font doesn't contain a required glyph");
  }

  if (f->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    throw FreetypeException_c("This text library can only handle outline fonts");
  }

  if (lib->outlineRender(&f->glyph->outline, params))
  {
    throw FreetypeException_c("The rendering of a glyph failed");
  }
}

FT_Face freeTypeLibrary_c::newFace(const fontRessource_c & res, uint32_t size)
{
  FT_Face f;
  if (FT_Open_Face(lib, &res, 0, &f))
  {
    throw FreetypeException_c(std::string("Could not open Font '") + res.getDescription() + "' maybe "
                              "file is spelled wrong or file is broken");
  }

  if (FT_Set_Char_Size(f, 0, size, 72, 72))
  {
    doneFace(f);

    throw FreetypeException_c(std::string("Could not set the requested file to font '") +
                              res.getDescription() + "'");
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
                                  res.getDescription() + "'. Maybe the font doesn't have one?");
      }
      return f;
    }
  }

  doneFace(f);
  throw FreetypeException_c(std::string("Could not find a unicode character map to font '") +
                            res.getDescription() + "'. Maybe the font doesn't have one?");
}

freeTypeLibrary_c::~freeTypeLibrary_c()
{
  FT_Done_FreeType(lib);
}

freeTypeLibrary_c::freeTypeLibrary_c()
{
  if (FT_Init_FreeType(&lib))
  {
    throw FreetypeException_c("Could not initialize font rendering library instance");
  }
}

void fontFamily_c::addFont(const fontRessource_c & res, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  fontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  fonts[par] = res;
}

std::shared_ptr<fontFace_c> fontFamily_c::getFont(uint32_t size, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  fontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  auto i = fonts.find(par);

  // when the font is not found, return an empty pointer
  if (i == fonts.end())
  {
    return std::shared_ptr<fontFace_c>();
  }
  else
  {
    return cache->getFont(i->second, size);
  }
}

}
