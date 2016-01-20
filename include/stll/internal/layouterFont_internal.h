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
#ifndef STLL_LAYOUTER_FONT_INT_H
#define STLL_LAYOUTER_FONT_INT_H

#include <string>
#include <memory>

#include <stdint.h>

namespace STLL { namespace internal {

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

} }

#endif
