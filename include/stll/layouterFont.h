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
#ifndef STLL_LAYOUTER_FONT_H
#define STLL_LAYOUTER_FONT_H

/** \file
 *  \brief a FreeType wrapper
 *
 * You normally don't need to use the stuff in this header
 */

#include <boost/utility.hpp>

#include <string>
#include <memory>
#include <map>
#include <vector>

#include <stdint.h>
#include <stdexcept>

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;

namespace STLL {

/** \brief type used for all glyph indices. Right now there is no
 * font with more than 2^16 fonts, so 2^32 should be on the safe side.
 * Also HarfBuzz also uses only 2^32 codepoints.
 */
typedef uint32_t glyphIndex_t;


/** \brief define, which sub-pixel arrangement you want to use for
 *  sub-pixel output
 */
typedef enum
{
  SUBP_NONE,   ///< don't use sub-pixel output (e.g. non LCD)
  SUBP_RGB,    ///< use horizontal RGB
  SUBP_BGR,    ///< use horizontal BGR
  SUBP_RGB_V,  ///< use vertical RGB (top to bottom)
  SUBP_BGR_V   ///< use vertical BGR (top to bottom)
} SubPixelArrangement;

/** \brief This class is thrown on problems with the FreeType library
 */
class FreetypeException_c : public std::runtime_error
{
  public:
    explicit FreetypeException_c(const std::string & what_arg) : std::runtime_error(what_arg) {}
};

class FreeTypeLibrary_c;

namespace internal {

/** \brief encapsulate information for one font file
 */
class FontFileResource_c
{
  private:
    std::shared_ptr<uint8_t> data; ///< pointer to an already loaded font file
    size_t datasize;               ///< size of that data chunk
    std::string descr;             ///< either a complete path to a font file, or a short description for the loaded file

  public:

    /** \brief create a font resource for font in RAM
     * \param p pointer to the RAM data
     * \param s size in bytes of the RAM data
     * \param d description of the font... not used by the library, you may use it for yourself to recognize fonts
     */
    FontFileResource_c(std::shared_ptr<uint8_t> p, size_t s, std::string d) : data(std::move(p)), datasize(s), descr(std::move(d)) {}

    /** \brief create a font resource for a font in a file
     * \param filename the filename to use
     */
    FontFileResource_c(std::string filename) : datasize(0), descr(std::move(filename)) {}

    /** Get the font description or the font file */
    const std::string & getDescription(void) const { return descr; }

    /** get the data, if nullptr, then description contains a file name */
    std::shared_ptr<uint8_t> getData(void) const { return data; }

    /** get the size of the data returned in getData */
    size_t getDatasize(void) const { return datasize; }

    /** comparison operator for stl containert */
    bool operator<(const FontFileResource_c & b) const
    {
      if (data < b.data) return true;
      if (data > b.data) return false;

      if (datasize < b.datasize) return true;
      if (datasize > b.datasize) return false;

      if (descr < b.descr) return true;
      return false;
    }
};

}


/** \brief This class represents a font resource.
 *
 *  A font is a collection of several font files that together constitute of the font. If several
 *  files contain the same glyphs then the first one added to this resource class will be used
 *
 *  A good example for the usage of this is the google noto font. Which consists of many files
 *  but the hebrew fonts don't contain numbers for example, so you have to take the glyphs for
 *  numbers from the regular fonts with the roman letters
 */
class FontResource_c
{
  private:
    std::vector<internal::FontFileResource_c> resources;

  public:
    /** create a new font resource with one font
     */
    template <typename... Args>
    FontResource_c(Args... par) { addFont(par...); }

    /** add another font to the resource: a file given its path.
     *
     * \param pathname File name of the file to use as font
     */
    void addFont(const std::string & pathname)
    {
      resources.emplace_back(internal::FontFileResource_c(pathname));
    };

    /** add another font to the resource: from memory.
     *
     * \param data is a pair containing a shared pointer to the data and the size of the data
     * \param descr description string, will be used to describe the font when an exception is thrown
     *              (e.g. problems loading the font)
     */
    void addFont(std::pair<std::shared_ptr<uint8_t>, size_t> data, const std::string & descr)
    {
      resources.emplace_back(internal::FontFileResource_c(data.first, data.second, descr));
    }

    /** the number of font files in this font
     */
    size_t size(void) const { return resources.size(); }

    /** iterators for for loops */
    auto begin(void) const { return resources.begin(); }
    auto end(void) const { return resources.end(); }

    /** Create an empty resource
     *
     * This constructor is required by the STL containers, don't use it
     */
    FontResource_c(void) {}

    /** get the fontfile ressource class for the fontfile at a given index
     */
    internal::FontFileResource_c getRessource(size_t idx) const { return resources[idx]; }

    /** operator for stl container usage */
    bool operator<(const FontResource_c & b) const
    {
      return std::lexicographical_compare(resources.begin(), resources.end(), b.resources.begin(), b.resources.end());
    }
};

/** \brief This class represents one font, made out of one font file resource with a certain size.
 */
class FontFace_c : boost::noncopyable
{
  public:

    class GlyphSlot_c
    {
      public:
        int w, h;
        int top, left;
        int pitch;
        const uint8_t * data;

        GlyphSlot_c(const FT_GlyphSlotRec_ * g);
        GlyphSlot_c(int width, int height) : w(width), h(height), top(0), left(0), pitch(0), data(0) {}
    };

    FontFace_c(std::shared_ptr<FreeTypeLibrary_c> l, const internal::FontFileResource_c & r, uint32_t size);
    ~FontFace_c();

    /** \brief Get the FreeType structure for this font
     *
     * This is required for example for harfbuzz. You normally don't need this when using STLL
     */
    FT_FaceRec_ * getFace(void) const { return f; }

    /** \name Functions to get font metrics
     *  @{ */

    /** \brief get the size of the font
     */
    uint32_t getSize(void) const { return size; }

    /** \brief get the font resource that was used to create this font
     */
    const internal::FontFileResource_c & getResource(void) const { return rec; }

    /** \brief Get the height of the font with multiplication factor of 64
     * \return height of font
     */
    uint32_t getHeight(void) const;

    /** \brief Get the ascender of the font with multiplication factor of 64
     * \return ascender of font
     */
    int32_t getAscender(void) const;

    /** \brief Get the descender of the font with multiplication factor of 64
     * \return descender of font
     */
    int32_t getDescender(void) const;

    /** \brief Get the underline position of the font with multiplication factor of 64
     * \return centre position of the underline relative to baseline
     */
    int32_t getUnderlinePosition(void) const;

    /** \brief Get the underline thickness of the font with multiplication factor of 64
     * \return thickness around the underline position
     */
    int32_t getUnderlineThickness(void) const;
    /** @} */

    /** \brief render a glyph of this font
     * \param glyphIndex the index of the glyph to render (take it from the layout)
     * \param sp the requested sub-pixel arrangement to apply to the rendering
     * \return a pointer to the FreeType glyph slot record, see FreeType documentation
     * \note glyphs are always rendered unhinted 8-bit FreeType bitmaps
     */
    GlyphSlot_c renderGlyph(glyphIndex_t glyphIndex, SubPixelArrangement sp);

    /** \brief check if a given character is available within this font
     * \param ch the unicode character to check
     * \return true, when the character is available within the font, false otherwise
     */
    bool containsGlyph(char32_t ch);

  private:
    FT_FaceRec_ *f;
    std::shared_ptr<FreeTypeLibrary_c> lib;
    internal::FontFileResource_c rec;
    uint32_t size;
};

/** \brief contains all the FontFaces_c of one FontRessource_c
 *
 * You usually don't create this class, it is greated for you by FontCache_c::getFont
 */
class Font_c
{
  public:

    Font_c(void) {}

    /** add a font face to the font */
    void add(std::shared_ptr<FontFace_c> f) { fonts.emplace_back(std::move(f)); }

    /** iterators for for loops */
    auto begin(void) const { return fonts.begin(); }
    auto end(void) const { return fonts.end(); }

    /** \brief find the fontface that contains the codepoint
     */
    std::shared_ptr<FontFace_c> get(char32_t codepoint) const;

    // some functions that get metrics of this font, they are always taken from the
    // first font face in the font
    uint32_t getHeight(void) const { return fonts[0]->getHeight(); }
    int32_t getAscender(void) const { return fonts[0]->getAscender(); }
    int32_t getDescender(void) const { return fonts[0]->getDescender(); }
    int32_t getUnderlinePosition(void) const { return fonts[0]->getUnderlinePosition(); }
    int32_t getUnderlineThickness(void) const { return fonts[0]->getUnderlineThickness(); }

    /** number of font faces in the font */
    explicit operator bool() const { return fonts.size() > 0; }

    /** comparison operator */
    bool operator==(const Font_c & rhs) const { return std::equal(fonts.begin(), fonts.end(), rhs.fonts.begin(), rhs.fonts.end()); }

  private:
    std::vector<std::shared_ptr<FontFace_c>> fonts;
};

/** \brief This class encapsulates an instance of the FreeType library
 *
 * The class exposes the functions of the FreeType library instance that are
 * required by STLL. You normally don't need to use this interface at all.
 *
 * You may need to create an instance of this class though, when you use the
 * low level interface (see \ref tutorial_pg)
 */
class FreeTypeLibrary_c : boost::noncopyable
{
  public:

    /** \brief Create an instance of the FreeType library */
    FreeTypeLibrary_c();
    /** \brief Destroy the library instance */
    ~FreeTypeLibrary_c();

    /** Make the library create a new font face using the given resource and size
     *
     * Usually you don't use this function directly but you use the FontFace_c class
     *
     * \param res The resource to use to create the font
     * \param size The requested font size
     * \return The FT_Face value
     */
    FT_FaceRec_ * newFace(const internal::FontFileResource_c & r, uint32_t size);

    /** Make the library destroy a font
     *
     * Usually you don't use this function directly but you use the FontFace_c class
     *
     * \param f the font to destroy
     */
    void doneFace(FT_FaceRec_ * f);

  private:

    FT_LibraryRec_ *lib;
};

/** \brief this class encapsulates open fonts of a single library, it makes
 *  sure that each font is open only once
 */
class FontCache_c
{
  public:

    /** \brief Create a cache using a specific library.
     *
     * I am not really sure where is might be useful to use multiple caches on one
     * library... though
     */
    FontCache_c(std::shared_ptr<FreeTypeLibrary_c> l) : lib(l) {}

    /** \brief Create a cache using an instance of the FreeType library that is created
     * specifically for this cache instance
     *
     * This is usually the thing you need, create one instance of this
     * class per thread of your application that needs to access the library
     */
    FontCache_c(void) : lib(std::make_shared<FreeTypeLibrary_c>()) {}

    /** \brief Get a font face from this cache with the given resource and size.
     *
     * If there is already one instance of this font open, it will be used
     * otherwise a new one will be opened.
     *
     * \param res The resource to use to create the font instance
     * \param idx the index, which font do we want to get from the ressource
     * \param size The requested size
     * \return The instance of the font face
     */
    Font_c getFont(const FontResource_c & res, uint32_t size);

    std::shared_ptr<FontFace_c> getFont(const internal::FontFileResource_c & res, uint32_t size);

    /** \brief remove all fonts from the cache, fonts that are still in use will be kept, but all others
     * are removed
     */
    void clear(void)
    {
      for(auto it = fonts.begin(); it != fonts.end(); )
      {
        if(it->second.use_count() == 1)
        {
          it = fonts.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }

  private:

    class FontFaceParameter_c
    {
      public:
        internal::FontFileResource_c res;
        uint32_t size;

        FontFaceParameter_c(const internal::FontFileResource_c r, uint32_t s) : res(std::move(r)), size(s) {}

        bool operator<(const FontFaceParameter_c & b) const
        {
          if (res < b.res) return true;
          if (b.res < res) return false;

          if (size < b.size) return true;
          return false;
        }
    };

    // all open fonts, used to check whether they have all been released
    // on library destruction
    std::map<FontFaceParameter_c, std::shared_ptr<FontFace_c> > fonts;

    // the library to use
    std::shared_ptr<FreeTypeLibrary_c> lib;
};

/** \brief a class contains all resources for a family of fonts
 *
 * A family is a set of fonts with roman, italics, bold, ... variants
 * this class will store the resources to each font and create a FontFace_c for
 * it, when requested.
 *
 * Usually you don't need to use this class directly as the CSS stylesheet class
 * TextStyleSheet_c will take care of this.
 */
class FontFamily_c
{
  private:

    // a class containing all the variables for a
    // font inside a family, it is used as the key inside the map
    // for all font files
    class FontFamilyParameter_c
    {
      public:
        std::string style;
        std::string variant;
        std::string weight;
        std::string stretch;

        bool operator<(const FontFamilyParameter_c & b) const
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

    /** \brief Initialize an empty family, using the given font cache to get the fonts
     */
    FontFamily_c(std::shared_ptr<FontCache_c> c) : cache(c) {}

    /** \brief Initialize an empty family, using font cache created specifically for this
     * family.
     *
     * This is normally not what you want as you will probably have
     * more than one family and they all should use the same cache
     */
    FontFamily_c(void) : cache(std::make_shared<FontCache_c>()) {}

    /** \brief Get a font instance from the family.
     *
     * \param size Size in pixels
     * \param style Font style, values are usually: "normal", "italic", "oblique"
     * \param variant Font variant, values are usually: "normal", "small-caps"
     * \param weight Font weight, typical values are: "lighter", "normal", "bold", "bolder"
     * \param stretch Font stretch, typical values are: "normal", "condensed"
     * \returns a nullptr, when the requested face doesn't exist, the font otherwise
     */
    Font_c getFont(uint32_t size,
                   const std::string & style = "normal",
                   const std::string & variant = "normal",
                   const std::string & weight = "normal",
                   const std::string & stretch = "normal");

    /** \brief Add a font to the family
     *
     * \param res Resource to use for this member of the font family
     * \param style Font style, values are usually: "normal", "italic", "oblique"
     * \param variant Font variant, values are usually: "normal", "small-caps"
     * \param weight Font weight, typical values are: "lighter", "normal", "bold", "bolder"
     * \param stretch Font stretch, typical values are: "normal", "condensed"
     */
    void addFont(const FontResource_c & res,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

  private:
    std::map<FontFamilyParameter_c, FontResource_c> fonts;
    std::shared_ptr<FontCache_c> cache;
};


}

#endif
