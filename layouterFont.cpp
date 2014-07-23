#include "layouterFont.h"

#include <hb.h>
#include <hb-ft.h>

#include <vector>
#include <string>
#include <memory>


// TODO the fontFace_c constructor and library interface is not perfect...
// TODO get rid of all asserts, we may do nothing on error, but we should
//      not exit the program


fontFace_c::fontFace_c(std::shared_ptr<freeTypeLibrary_c> l, const std::string & fname, uint32_t size) : lib(l)
{
  printf("font face const\n");
  f = lib->newFace(fname, size);
}

fontFace_c::~fontFace_c()
{
  printf("font face dest\n");
  lib->doneFace(f);
}

std::shared_ptr<fontFace_c> fontCache_c::getFont(const std::string fname, uint32_t size)
{
  fontFaceParameter_c ffp(fname, size);

  auto i = fonts.find(ffp);

  if (i != fonts.end())
  {
    auto a = i->second.lock();

    if (a)
      return a;
  }

  // TODO kann es sein, dass noch jeman anderes hier den Font öffnet?

  auto f = std::make_shared<fontFace_c>(lib, fname, size);

  fonts[ffp] = f;

  return f;
}



FT_Error fontFace_c::outlineRender(uint32_t idx, FT_Raster_Params* params)
{
  FT_Error fterr = FT_Load_Glyph(f, idx, 0);

  if (fterr == 0)
  {
    if (f->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    {
      return 0; // TODO return error that the glyph has not the right format
    }
    else
    {
      fterr = lib->outlineRender(&f->glyph->outline, params);
    }
  }

  return fterr;
}

FT_Face freeTypeLibrary_c::newFace(const std::string fname, uint32_t size)
{
  FT_Face f;
  assert(!FT_New_Face(lib, fname.c_str(), 0, &f));
  assert(!FT_Set_Char_Size(f, 0, size, 72, 72));

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
      assert(!FT_Set_Charmap(f, f->charmaps[i]));
      return f;
    }
  }

  assert(0);
}

freeTypeLibrary_c::~freeTypeLibrary_c()
{
  printf("delete library\n");
  FT_Done_FreeType(lib);
}

freeTypeLibrary_c::freeTypeLibrary_c()
{
  assert(!FT_Init_FreeType(&lib));
}

void fontFamily_c::addFont(const std::string& file, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  fontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  // make sure we only add once
  assert(fonts.find(par) == fonts.end());

  fonts.insert(std::make_pair(par, file));
}

std::shared_ptr<fontFace_c> fontFamily_c::getFont(uint32_t size, const std::string& style, const std::string& variant, const std::string& weight, const std::string& stretch)
{
  fontFamilyParameter_c par;

  par.style = style;
  par.variant = variant;
  par.weight = weight;
  par.stretch = stretch;

  auto i = fonts.find(par);

  if (i == fonts.end())
  {
    return std::shared_ptr<fontFace_c>();
  }
  else
  {
    return cache->getFont(i->second, size);
  }
}

