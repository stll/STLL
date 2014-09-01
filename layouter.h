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
#ifndef __LAYOUTER_H__
#define __LAYOUTER_H__

/** \file
 *  \brief the paragraph layouter and its helpers
 */

#include "layouterFont.h"

#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/closed_interval.hpp>

#include <string>
#include <vector>
#include <memory>

/** \brief The namespace for the library.
 *  Every function and class is within this namespace.
 */
namespace STLL {

/** \brief encapsulates a finished layout.
 *
 * This class encapsulates a layout, it is a list of drawing commands.
 */
class textLayout_c
{
  private:
    uint32_t height; // position where you can add additional text after this ...
    int32_t left, right;

  public:

    /** \brief This structure encapsulates a drawing command
     */
    typedef struct
    {
      enum
      {
        CMD_GLYPH,  ///< draw a glyph from a font
        CMD_RECT    ///< draw a rectangle
      } command;    ///< specifies what to draw

      /** \name position of the glyph, or upper left corner of rectangle
       *  @{ */
      int32_t x;  ///< x position
      int32_t y;  ///< y position
      /** @} */

      /** \brief which glyph to draw */
      FT_UInt glyphIndex;

      /** \brief which front to take the glyph from */
      std::shared_ptr<fontFace_c> font;

      /** \name width and height of the rectangle to draw
       *  @{ */
      uint32_t w;  ///< width of block
      uint32_t h;  ///< height of block
      /** @} */

      /** \name colour of the glyph or the rectangle
       *  @{ */
      uint8_t r;  ///< red value
      uint8_t g;  ///< green value
      uint8_t b;  ///< blue value
      uint8_t a;  ///< alpha value (0: transparent, 255: opaque)
      /** @} */

    } commandData;

    // order is the logical order, so when you do copy and paste we copy out of this using the order
    // within the vector
    // TODO find better solution instead of public vector
    std::vector<commandData> data;

    /** \brief add a single drawing command to the end of the command list
     *  \param d the command to add
     */
    void addCommand(const commandData & d)
    {
      data.push_back(d);
    }

    /** \brief add a single drawing command to the start of the command list
     *  \param d the command to add
     */
    void addCommandStart(const commandData & d)
    {
      data.insert(data.begin(), d);
    }

    /** \brief add whole vector of commands to the layout, the commands are offset
     *         by a certain amount to shift everything that is drawn
     *  \param d the commands to add
     *  \param dx the x offset
     *  \param dy the y offset
     */
    void addCommandVector(const std::vector<commandData> & d, int dx, int dy)
    {
      for (auto a : d)
      {
        a.x += dx;
        a.y += dy;
        data.emplace_back(a);
      }
    }

    /** \brief append a layout to this layout, which means that the drawing
     *         commands of the 2nd layout are copied into this layout
     *  \param l the layout to append
     */
    void append(const textLayout_c & l)
    {
      for (auto & a : l.data)
        data.push_back(a);

      height = std::max(height, l.height);
    }

    /** \brief move assignment
     */
    void operator=(textLayout_c && l)
    {
      data.swap(l.data);
      height = l.height;
    }

    /** \brief copy assignment
     */
    void operator=(const textLayout_c & l)
    {
      data = l.data;
      height = l.height;
    }

    ~textLayout_c(void) { }

    /** \brief create empty layout
     */
    textLayout_c(void) : height(0) { }

    /** \brief copy constructor
     */
    textLayout_c(const textLayout_c & src) : height(src.height), data(src.data)  {  }

    /** \brief move constructor
     */
    textLayout_c(textLayout_c && src)
    {
      swap(data, src.data);
      height = src.height;
    }

    /** \brief the height of the layout. This is supposed to be the vertical
     *  space that this layout takes up
     */
    uint32_t getHeight(void) const { return height; }

    /** \brief set the height
     */
    void setHeight(uint32_t h) { height = h; }

    /** \brief the width of the layout. This is supposed to be the horizontal
     *  space that this layout takes up
     */
    int32_t getLeft(void) const { return left; }

    /** \brief set the width
     */
    void setLeft(int32_t l) { left = l; }

    /** \brief the width of the layout. This is supposed to be the horizontal
     *  space that this layout takes up
     */
    int32_t getRight(void) const { return right; }

    /** \brief set the width
     */
    void setRight(int32_t r) { right = r; }
};

/** \brief this structure contains all attributes that a single glyph can get assigned
 */
class codepointAttributes
{
  public:

  //@{
  /// color of the letter, r, g, b values and alpha (0: transparent 255: opaque)
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
  //@}

  /// font of the letter
  std::shared_ptr<fontFace_c> font;

  /// current language at this letter
  std::string lang;

  static const uint8_t FL_UNDERLINE = 1;

  uint8_t flags;

  typedef struct shadow
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    int8_t dx;
    int8_t dy;

    bool operator==(const struct shadow & rhs) const
    {
      return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a && dx == rhs.dx && dy == rhs.dy;
    }

  } shadow;

  std::vector<shadow> shadows;

  bool operator==(const codepointAttributes & rhs) const
  {
    return r == rhs.r && g == rhs.g && b == rhs.b && font == rhs.font && lang == rhs.lang
      && flags == rhs.flags && shadows.size() && rhs.shadows.size()
      && std::equal(shadows.begin(), shadows.end(), rhs.shadows.begin());
  }

  codepointAttributes operator += (const codepointAttributes & rhs)
  {
    // when someone tries to overwrite the attributes, we take over the new ones
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
    font = rhs.font;
    lang = rhs.lang;
    flags = rhs.flags;
    shadows = rhs.shadows;

    return *this;
  }
};


/** \brief collection of codepointAttributes for the layouter.
 *
 * This class behaves a bit like a vector of codepointAttributed in that you can
 * get an attribute for an index. The index is of type size_t
 */
class attributeIndex_c
{
  private:
    boost::icl::interval_map<size_t, codepointAttributes> attr;

  public:
    /** \brief create an empty index
     *  \attention you have to add at least one attribute
     */
    attributeIndex_c(void) { }

    /** \brief create an index where all entries have the same attribute
     *  \param a the attribute that all will share
     */
    attributeIndex_c(const codepointAttributes & a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(0, SIZE_MAX), a);
    }

    /** \brief set attributes for a singe of indices
     *  \param i index that will have the attribute
     *  \param a the attribute
     */
    void set(size_t i, codepointAttributes a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(i, i), a);
    }

    /** \brief set attributes for a range of indices
     *  \param start first index that should have the attribute
     *  \param end first index that will no longer have the attribute
     *  \param a the attribute
     */
    void set(size_t start, size_t end, codepointAttributes a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(start, end), a);
    }

    /** \brief get the attribute for given index
     *  \param i the index for which the attribute is requested
     *  \return the requested attribute
     */
    const codepointAttributes & get(size_t i) const
    {
      return attr.find(i)->second;
    }
};

/** \brief base class to define the shape to layout text into
 *
 * The layouting works line-wise, this class will give the left and right edge of
 * the available space for a line of text. As the line has a vertical size the
 * functions in this class will get the top and bottom of the y-area that the line is
 * supposed to be in. The function then need to check where the left and right edges
 * are for this area, e.g. they must take care that they return the smallest area.
 */
class shape_c
{
  public:
    /** \brief get the left edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the left edge usable for this section of the y-axis
     */
    virtual int32_t getLeft(int32_t top, int32_t bottom) const = 0;

    /** \brief get the right edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the right edge usable for this section of the y-axis
     */
    virtual int32_t getRight(int32_t top, int32_t bottom) const = 0;

    /** \brief get the left edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the left outer edge for this section of the y-axis
     */
    virtual int32_t getLeft2(int32_t top, int32_t bottom) const = 0;

    /** \brief get the right edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the right outer edge for this section of the y-axis
     */
    virtual int32_t getRight2(int32_t top, int32_t bottom) const = 0;
};

/** \brief conctrete implementation of the shape that will allow layouting
 *  inside a rectangular with a certain width
 */
class rectangleShape_c : public shape_c
{
  private:
    int32_t w;

  public:
    /** \brief construct the shape such that the area goes from 0 to width on the x-axis
     *  \param width width of the rectangle
     */
    rectangleShape_c(int32_t width) : w(width) { }

    virtual int32_t getLeft(int32_t /*top*/, int32_t /*bottom*/) const { return 0; }
    virtual int32_t getLeft2(int32_t /*top*/, int32_t /*bottom*/) const { return 0; }
    virtual int32_t getRight(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
    virtual int32_t getRight2(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
};

/** \brief this structure contains information for the layouter how to layout the text
 */
typedef struct {
  enum {
    ALG_LEFT,          ///< layout left adjusted
    ALG_RIGHT,         ///< layout right adjusted
    ALG_CENTER,        ///< layout centered
    ALG_JUSTIFY_LEFT,  ///< layout justified and the last line left adjusted
    ALG_JUSTIFY_RIGHT  ///< layout justified and the last line right adjusted
  } align;             ///< alignment that the text is supposed to have

  int32_t indent;      ///< indentation of the first line in pixel

  bool ltr;            ///< is the base direction of the text left to right?

} layoutProperties;


/** paragraph layouting function
 *
 * \param txt32 the utf-32 encoded text to layout, no control sequences exist, use "\n" for newlines
 * \param attr the attributes (colors, ...) for all the characters in the text
 * \param shape the shape that the final result is supposed to have
 * \param prop some parameters that the line breaking algorithm needs to give the result the expected
 *             shape (center or justified, ...)
 * \param ystart the vertical starting point of your output, the baseline of the first line
 *               of text will be shifted down from this position by the ascender of the line
 * \return the resulting layout
 */
textLayout_c layoutParagraph(const std::u32string & txt32, const attributeIndex_c & attr,
                             const shape_c & shape, const layoutProperties & prop, int32_t ystart = 0);

}

#endif
