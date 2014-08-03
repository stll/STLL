#ifndef __LAYOUTER_FONT_H__
#define __LAYOUTER_FONT_H__

/** \file
 *  \brief a freetype wrapper
 */

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <boost/utility.hpp>

#include <string>
#include <memory>
#include <assert.h>
#include <vector>
#include <map>

#include <stdint.h>

#include <stdexcept>

/** \brief exception thrown on problems with the freetype library
 *
 * You normally don't need to use the stuff in this header
 */
class FreetypeException_c : public std::runtime_error
{
  public:
    explicit FreetypeException_c(const std::string & what_arg) : std::runtime_error(what_arg) {}

};

class freeTypeLibrary_c;

/** \brief one font, made out of one file with a certain size
 */
class fontFace_c : boost::noncopyable
{
  public:

    fontFace_c(std::shared_ptr<freeTypeLibrary_c> l, const std::string & fname, uint32_t size);
    ~fontFace_c();

    void outlineRender(uint32_t idx, FT_Raster_Params * params);

    FT_Face getFace(void) const { return f; }

    uint32_t getHeigt(void) const { return f->size->metrics.height; }
    int32_t getAscender(void) const { return f->size->metrics.ascender; }
    int32_t getDescender(void) const { return f->size->metrics.descender; }

  private:
    FT_Face f;
    std::shared_ptr<freeTypeLibrary_c> lib;
};

/** \brief access to the functionality of the freetype library
 */
class freeTypeLibrary_c : boost::noncopyable
{
  public:

    freeTypeLibrary_c();
    ~freeTypeLibrary_c();

    FT_Face newFace(const std::string fname, uint32_t size);

    FT_Error outlineRender(FT_Outline * o, FT_Raster_Params * params)
    {
      return FT_Outline_Render(lib, o, params);
    }

    void doneFace(FT_Face f) { FT_Done_Face(f); }

  private:

    FT_Library lib;
};

/** \brief this class encapsulates fonts of a single library, it makes
 *  sure that each font is opened only once as long as it is open
 */
class fontCache_c
{
  public:

    // create a cache using a specific library, I am not really
    // sure where is might be useful to use multiple caches on one
    // library... though
    fontCache_c(std::shared_ptr<freeTypeLibrary_c> l) : lib(l) {}

    // create a cache using a self made instance of the freetype library
    // this is usually the thing you need, create one instance of this
    // class per thread of your application that needs to access the library
    fontCache_c(void) : lib(std::make_shared<freeTypeLibrary_c>()) {}

    // get a font face from this cache with the given name and size attribute
    // if there is already one instance of this font open, it will be used
    // otherwise a new one will be opened
    std::shared_ptr<fontFace_c> getFont(const std::string fname, uint32_t size);

  private:

    class fontFaceParameter_c
    {
    public:
      std::string fname;
      uint32_t size;

      fontFaceParameter_c(const std::string & f, uint32_t s) : fname(f), size(s) {}

      bool operator<(const fontFaceParameter_c & b) const
      {
        if (fname < b.fname) return true;
        if (fname > b.fname) return false;
        if (size < b.size) return true;
        return false;
      }
    };

    // all open fonts, used to check whether they have all been released
    // on library destruction
    std::map<fontFaceParameter_c, std::weak_ptr<fontFace_c> > fonts;

    // the library to use
    std::shared_ptr<freeTypeLibrary_c> lib;
};

// a class contains all file names for a family of fonts
// a family is a set of fonts with roman, italics, bold, ... variants
// this class will store the filenames to each font file
// and open create a fontFace_c for it, whenn requested
class fontFamily_c
{
  private:

    // a class containing all the variables for a
    // font inside a family, it is used as the key inside the map
    // for all font files
    class fontFamilyParameter_c
    {
      public:
        std::string style;
        std::string variant;
        std::string weight;
        std::string stretch;

        bool operator<(const fontFamilyParameter_c & b) const
        {
          if (style < b.style) return true;
          if (style > b.style) return false;
          if (variant < b.variant) return true;
          if (variant > b.variant) return false;
          if (weight < b.weight) return true;
          if (weight > b.weight) return false;
          if (stretch < b.stretch) return true;
          return false;
        }
    };

  public:

    // initialize an empty family, using the given font cache
    // to get the fonts
    fontFamily_c(std::shared_ptr<fontCache_c> c) : cache(c) {}

    // initialize using a self made cache with a self made cache
    // this is normally not what you want as you will probably have
    // more than one family and they all should use the same cache
    fontFamily_c(void) : cache(std::make_shared<fontCache_c>()) {}

    // size: in pixel only
    // style: normal, italic, oblique
    // variant: normal, small-caps
    // weight: lighter, normal, bold, bolder
    // normal, condensed
    // returns a nullptr, when the requested face doesn't exist
    std::shared_ptr<fontFace_c> getFont(uint32_t size,
                                        const std::string & style = "normal",
                                        const std::string & variant = "normal",
                                        const std::string & weight = "normal",
                                        const std::string & stretch = "normal");

    // add a font to the family
    void addFont(const std::string & file,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

    // return the cache used by this family
    std::shared_ptr<fontCache_c> getCache(void) { return cache; }

  private:
    std::map<fontFamilyParameter_c, std::string> fonts;
    std::shared_ptr<fontCache_c> cache;
};


#endif
