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
#ifndef __LAYOUTER_FONT_H__
#define __LAYOUTER_FONT_H__

/** \file
 *  \brief a freetype wrapper
 *
 * You normally don't need to use the stuff in this header
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

namespace STLL {

/** \brief This class is thrown on problems with the freetype library
 */
class FreetypeException_c : public std::runtime_error
{
  public:
    explicit FreetypeException_c(const std::string & what_arg) : std::runtime_error(what_arg) {}
};

class freeTypeLibrary_c;

/** \brief This class represents a font resource.
 *
 *  Right now a file name or a memory pointer are supported resource types
 */
class fontResource_c : public FT_Open_Args
{
  private:
    std::string v;
    std::shared_ptr<uint8_t> dat;

  public:

    /** Create a font resource for a file given its path.
     *
     * \param fname File name of the file to use as font
     */
    fontResource_c(const std::string & fname);

    /** Create a font resource from memory.
     *
     * \param data is a pair containing a shared pointer to the data and the size of the data
     * \param descr description string, will be used to describe the font when an exception is thrown
     *              (e.g. problems loading the font)
     */
    fontResource_c(std::pair<std::shared_ptr<uint8_t>, size_t> data, const std::string & descr);

    /** Create an empty resource
     *
     * This constructor is required by the STL containers, don't use it
     */
    fontResource_c(void)
    {
      flags = 0;
    }

    /** Get the description of the font resource.
     *
     * \return the description. This will be either the path to the font file, or the description
     * given when creating a memory resource
     */
    const std::string & getDescription(void) const { return v; }
};

/** \brief This class represents one font, made out of one resource and with a certain size.
 */
class fontFace_c : boost::noncopyable
{
  public:

    fontFace_c(std::shared_ptr<freeTypeLibrary_c> l, const fontResource_c & res, uint32_t size);
    ~fontFace_c();

    /** \brief Perform an outline rendering of a glyph of this font
     *
     * \param idx the index to the glyp to render (you get this from the textLayout_c class
     * \param params the rendering structure. It contains information where to render too (mainly callbacks)
     *
     * TODO wrap FT_Raster_Params
     */
    void outlineRender(uint32_t idx, FT_Raster_Params * params);

    /** \brief Get the freetype structure for this font
     *
     * This is required for example for harfbuzz. You normally don't need this when using STLL
     */
    FT_Face getFace(void) const { return f; }

    /** \name Functions to get font metrics
     *  @{ */

    /** \brief Get the height of the font with multiplication factor of 64
     * \return height of font
     */
    uint32_t getHeigt(void) const { return f->size->metrics.height; }

    /** \brief Get the ascender of the font with multiplication factor of 64
     * \return ascender of font
     */
    int32_t getAscender(void) const { return f->size->metrics.ascender; }

    /** \brief Get the descender of the font with multiplication factor of 64
     * \return descender of font
     */
    int32_t getDescender(void) const { return f->size->metrics.descender; }

    /** \brief Get the underline position of the font with multiplication factor of 64
     * \return centre position of the underline relative to baseline
     */
    int32_t getUnderlinePosition(void) const { return (int64_t)f->underline_position*f->size->metrics.y_scale / 65536; }

    /** \brief Get the underline thickness of the font with multiplication factor of 64
     * \return thickness around the underline position
     */
    int32_t getUnderlineThickness(void) const { return (int64_t)f->underline_thickness*f->size->metrics.y_scale / 65536; }
    /** @} */
  private:
    FT_Face f;
    std::shared_ptr<freeTypeLibrary_c> lib;
};

/** \brief This class encapsulates an instance of the freetype library
 *
 * The class exposes the functions of the freetype library instance that are
 * required by STLL. You normally don't need to use this interface at all.
 *
 * You may need to create an instance of this class though, when you use the
 * low level interface (see \ref tutorial_pg)
 */
class freeTypeLibrary_c : boost::noncopyable
{
  public:

    /** \brief Create an instance of the freetype library */
    freeTypeLibrary_c();
    /** \brief Destroy the library instance */
    ~freeTypeLibrary_c();

    /** Make the library create a new font face using the given ressource and size
     *
     * Usually you don't use this function directly but you use the fontFace_c class
     *
     * \param res The resource to use to create the font
     * \param size The requested font size
     * \return The FT_Face value
     */
    FT_Face newFace(const fontResource_c & res, uint32_t size);

    /** Make the library render an outline of a glyph
     *
     * Usually you don't use this function directly but you use the fontFace_c class
     *
     * \param o The outline to render
     * \param params The rendering parameters
     * \return Error code
     */
    FT_Error outlineRender(FT_Outline * o, FT_Raster_Params * params)
    {
      return FT_Outline_Render(lib, o, params);
    }

    /** Make the library destroy a font
     *
     * Usually you don't use this function directly but you use the fontFace_c class
     *
     * \param f the font to destroy
     */
    void doneFace(FT_Face f) { FT_Done_Face(f); }

  private:

    FT_Library lib;
};

/** \brief this class encapsulates open fonts of a single library, it makes
 *  sure that each font is open only once
 */
class fontCache_c
{
  public:

    /** \brief Create a cache using a specific library.
     *
     * I am not really sure where is might be useful to use multiple caches on one
     * library... though
     */
    fontCache_c(std::shared_ptr<freeTypeLibrary_c> l) : lib(l) {}

    /** \brief Create a cache using an instance of the freetype library that is created
     * specifically for this cache instance
     *
     * This is usually the thing you need, create one instance of this
     * class per thread of your application that needs to access the library
     */
    fontCache_c(void) : lib(std::make_shared<freeTypeLibrary_c>()) {}

    /** Get a font face from this cache with the given resource and size.
     *
     * If there is already one instance of this font open, it will be used
     * otherwise a new one will be opened.
     *
     * \param res The resource to use to create the font instance
     * \param size The requested size
     * \return The instance of the font face
     */
    std::shared_ptr<fontFace_c> getFont(const fontResource_c & res, uint32_t size);

    /** Get the font resource for a font inside the cache (or empty resource, if the
     * font is not within
     *
     * \param f font to look for
     * \return the resource that was used to create the font
     */
    fontResource_c getFontResource(std::shared_ptr<fontFace_c> f) const;

    /** Get the font size for a font inside the cache (or zero, if the
     * font is not within
     *
     * \param f font to look for
     * \return the size used to create the font
     */
    uint32_t getFontSize(std::shared_ptr<fontFace_c> f) const;

  private:

    class fontFaceParameter_c
    {
    public:
      fontResource_c res;
      uint32_t size;

      fontFaceParameter_c(const fontResource_c & r, uint32_t s) : res(r), size(s) {}

      bool operator<(const fontFaceParameter_c & b) const
      {
        if (res.flags < b.res.flags) return true;
        if (res.flags > b.res.flags) return false;

        if (res.memory_base < b.res.memory_base) return true;
        if (res.memory_base > b.res.memory_base) return false;

        if (res.pathname < b.res.pathname) return true;
        if (res.pathname > b.res.pathname) return false;

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

/** \brief a class contains all resources for a family of fonts
 *
 * A family is a set of fonts with roman, italics, bold, ... variants
 * this class will store the resources to each font and create a fontFace_c for
 * it, when requested.
 *
 * Usually you don't need to use this class directly as the CSS stylesheet class
 * textStyleSheet_c will take care of this.
 */
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

    /** \brief Initialize an empty family, using the given font cache to get the fonts
     */
    fontFamily_c(std::shared_ptr<fontCache_c> c) : cache(c) {}

    /** \brief Initialize an empty family, using font cache created specifically for this
     * family.
     *
     * This is normally not what you want as you will probably have
     * more than one family and they all should use the same cache
     */
    fontFamily_c(void) : cache(std::make_shared<fontCache_c>()) {}

    /** \brief Get a font instance from the family.
     *
     * \param size Size in pixels
     * \param style Font style, values are usually: "normal", "italic", "oblique"
     * \param variant Font variant, values are usually: "normal", "small-caps"
     * \param weight Font weight, typical values are: "lighter", "normal", "bold", "bolder"
     * \param stretch Font stretch, typical values are: "normal", "condensed"
     * \returns a nullptr, when the requested face doesn't exist, the font otherwise
     */
    std::shared_ptr<fontFace_c> getFont(uint32_t size,
                                        const std::string & style = "normal",
                                        const std::string & variant = "normal",
                                        const std::string & weight = "normal",
                                        const std::string & stretch = "normal");

    /** \brief Add a font to the family
     *
     * \param res Ressource to use for this member of the font family
     * \param style Font style, values are usually: "normal", "italic", "oblique"
     * \param variant Font variant, values are usually: "normal", "small-caps"
     * \param weight Font weight, typical values are: "lighter", "normal", "bold", "bolder"
     * \param stretch Font stretch, typical values are: "normal", "condensed"
     */
    void addFont(const fontResource_c & res,
                 const std::string & style = "normal",
                 const std::string & variant = "normal",
                 const std::string & weight = "normal",
                 const std::string & stretch = "normal");

  private:
    std::map<fontFamilyParameter_c, fontResource_c> fonts;
    std::shared_ptr<fontCache_c> cache;
};

}

#endif
