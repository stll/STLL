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
#ifndef STLL_LAYOUTER_H
#define STLL_LAYOUTER_H

/** \file
 *  \brief the paragraph layouter and its helpers
 */

#include "layouterFont.h"

#include "color.h"

#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/closed_interval.hpp>

#include <string>
#include <vector>
#include <memory>

#include <stdint.h>

/** \brief The namespace for the library.
 *  Every function and class is within this namespace.
 */
namespace STLL {

/** \brief This structure encapsulates a drawing command
 */
class CommandData_c
{
public:
  enum
  {
    CMD_GLYPH,  ///< draw a glyph from a font
    CMD_RECT,   ///< draw a rectangle
    CMD_IMAGE   ///< draw an image
  } command;    ///< specifies what to draw

  /** \name position of the glyph, or upper left corner of rectangle or image
   *  @{ */
  int32_t x;  ///< x position
  int32_t y;  ///< y position
  /** @} */

  /** \brief which glyph to draw */
  glyphIndex_t glyphIndex;

  /** \brief which front to take the glyph from */
  std::shared_ptr<FontFace_c> font;

  /** \name width and height of the rectangle to draw
   *  @{ */
  uint32_t w;  ///< width of block
  uint32_t h;  ///< height of block
  /** @} */

  /** \brief colour of the glyph or the rectangle */
  Color_c c;

  /** \brief blurr radius to use for this command */
  uint16_t blurr;

  std::string imageURL; ///< URL of image to draw

  /** \brief constructor to create an glyph command
   */
  CommandData_c(std::shared_ptr<FontFace_c> f, glyphIndex_t i, int32_t x_, int32_t y_, Color_c c_, uint16_t rad) :
  command(CMD_GLYPH), x(x_), y(y_), glyphIndex(i), font(f), w(0), h(0), c(c_), blurr(rad) {}

  /** \brief constructor to create an image command
   */
  CommandData_c(const std::string & i, int32_t x_, int32_t y_, uint32_t w_, uint32_t h_) :
  command(CMD_IMAGE), x(x_), y(y_), glyphIndex(0), w(w_), h(h_), blurr(0), imageURL(i) {}

  /** \brief constructor to create an rectangle command
   */
  CommandData_c(int32_t x_, int32_t y_, uint32_t w_, uint32_t h_, Color_c c_, uint16_t rad) :
  command(CMD_RECT), x(x_), y(y_), glyphIndex(0), w(w_), h(h_), c(c_), blurr(rad) {}
};

/** \brief encapsulates a finished layout.
 *
 * This class encapsulates a layout, it is a list of drawing commands.
 */
class TextLayout_c
{
  private:
    // position where you can add additional text after this ...
    uint32_t height;
    // left and right edge
    int32_t left, right;
    // vertical position of the very first baseline in this layout
    int32_t firstBaseline;
    // the drawing commands that make up this layout
    std::vector<CommandData_c> data;

  public:

    /** \brief get the command vector
     */
    const std::vector<CommandData_c> getData(void) const { return data; }

    /** \brief a little structure to hold information for one rectangle */
    class Rectangle_c
    {
      public:
        int x, y, w, h;
    };

    /** \brief a helper structure to contain information about the interactive
     * area for a link */
    class LinkInformation_c
    {
      public:
        /** \brief the url for the link */
        std::string url;
        /** \brief the areas where the link is found */
        std::vector<Rectangle_c> areas;
    };

    /** \brief information for all links TODO this interface is bad */
    std::vector<LinkInformation_c> links;

    /** \brief add a single drawing command to the end of the command list
     *  \param args this is a forwarding function that will give all arguments to the
     * constructor of the CommandData_c class that will be added
     *
     * \note if you add a command, you must update the left, right and height fields
     * as well as the firstBaseline your own
     */
    template <class... Args>
    void addCommand(Args&&... args)
    {
      data.emplace_back(std::forward<Args>(args)...);
    }

    /** \brief add a single drawing command to the end of the command list
     *  \param c the command to add
     *
     * \note if you add a command, you must update the left, right and height fields
     * as well as the firstBaseline your own
     */
    void addCommand(const CommandData_c & c)
    {
      data.push_back(c);
    }

    /** \brief add a single drawing command to the start of the command list
     *  \param args this is a forwarding function that will give all arguments to the
     * constructor of the CommandData_c class that will be added
     *
     * \note if you add a command, you must update the left, right and height fields
     * as well as the firstBaseline your own
     */
    template <class... Args>
    void addCommandStart(Args&&... args)
    {
      data.emplace(data.begin(), std::forward<Args>(args)...);
    }

    /** \brief add a single drawing command to the start of the command list
     *  \param d the command to add
     *
     * \note if you add a command, you must update the left, right and height fields
     * as well as the firstBaseline your own
     */
    void addCommandStart(const CommandData_c & d)
    {
      data.insert(data.begin(), d);
    }

    /** \brief append a layout to this layout, which means that the drawing
     *         commands of the 2nd layout are copied into this layout
     *
     *  The height, left and right border are adjusted to completely accommodate
     *  all of the new layout, the firstBaseline is copied over, when this layout
     *  is currently empty, else it is left untouched
     *  \param l the layout to append
     *  \param dx x-offset to apply when appending the layout
     *  \param dy y-offset to apply when appending the layout
     */
    void append(const TextLayout_c & l, int dx = 0, int dy = 0);

    /** \brief move assignment
     */
    void operator=(TextLayout_c && l)
    {
      data.swap(l.data);
      height = l.height;
      left = l.left;
      right = l.right;
      firstBaseline = l.firstBaseline;
      swap(links, l.links);
    }

    /** \brief copy assignment
     */
    void operator=(const TextLayout_c & l)
    {
      data = l.data;
      height = l.height;
      left = l.left;
      right = l.right;
      firstBaseline = l.firstBaseline;
      links = l.links;
    }

    ~TextLayout_c(void) { }

    /** \brief create empty layout
     */
    TextLayout_c(void);

    /** \brief copy constructor
     */
    TextLayout_c(const TextLayout_c & src);

    /** \brief move constructor
     */
    TextLayout_c(TextLayout_c && src);

    /** \brief shift all the commands within the layout by the given amount
     *
     * \param dx x-offset in 1/64th pixels
     * \param dy y-offset in 1/64th pixels
     */
    void shift(int32_t dx, int32_t dy);

    /** \brief the height of the layout. This is supposed to be the vertical
     *  space that this layout takes up in 1/64th pixels
     */
    uint32_t getHeight(void) const { return height; }

    /** \brief set the height in 1/64th pixels
     */
    void setHeight(uint32_t h) { height = h; }

    /** \brief get the left edge of the layout in 1/64th pixels
     */
    int32_t getLeft(void) const { return left; }

    /** \brief set the left edge  in 1/64th pixels
     */
    void setLeft(int32_t l) { left = l; }

    /** \brief get the right edge of the layout in 1/64th pixels
     */
    int32_t getRight(void) const { return right; }

    /** \brief get the right edge in 1/64th pixels
     */
    void setRight(int32_t r) { right = r; }

    /** \brief set the y position of the first baseline
     */
    void setFirstBaseline(int32_t r) { firstBaseline = r; }

    /** \brief get the y position of the first baseline
     */
    int32_t getFirstBaseline(void) const { return firstBaseline; }
};

/** \brief this structure contains all attributes that a single glyph can get assigned
 */
class CodepointAttributes_c
{
  public:

  /** \brief colour of the letter
   */
  Color_c c;

  /** \brief font of the letter
   */
  Font_c font;

  /** \brief current language at this letter
   */
  std::string lang;

  /** \brief bit for the flags variable, if set the glyph will be underlined
   */
  static const uint8_t FL_UNDERLINE = 1;

  /** \brief some flags for the glyph, use FL_xxx constants to change the value
   */
  uint8_t flags;

  /** \brief one element of the shadow
   */
  class Shadow_c
  {
    public:

      /** \brief the colour to use for this shadow element
       */
      Color_c c;

      /** \brief offset of the shadow in 1/64th pixels relative to the
       * position of the normal glyph
       */
      int8_t dx;

      /** \brief offset of the shadow in 1/64th pixels relative to the
       * position of the normal glyph
       */
      int8_t dy;

      /** \brief radius of the shadow in 1/64th pixel size
       */
      int16_t blurr;

      /** \brief comparison operator for one shadow
       */
      bool operator==(const struct Shadow_c & rhs) const
      {
        return c == rhs.c && dx == rhs.dx && dy == rhs.dy && blurr == rhs.blurr;
      }
  };

  /** \brief the shadows to draw behind the real glyph. The shadows are drawn in the
   * order within this vector and on top of that the real glyph will be drawn
   * inlays will have no shadow
   */
  std::vector<Shadow_c> shadows;

  /** \brief sometimes you want to place something different instead of a glyph (e.g. an image).
   * This can be done with an inlay.
   *
   * The inlay is a layout that will be placed instead of an actual glyph.
   * If you define the inlay, the font information will be ignored.
   * How the inlay interacts with line breaks depends on the character that is given
   * as the place holder. Line breaks are done as if there was the given place holder
   * character and not the inlay.
   *
   * The vertical alignment of the inlay is controlled by baseline_shift. If
   * baseline_shift is 0 the inlay is placed on the baseline.
   */
  std::shared_ptr<TextLayout_c> inlay;

  /** \brief  do you want to move the baseline of this character or inlay relative to the
   * baseline of the line? A positive value means to move it up. It is given in units
   * of 1/64th pixel.
   */
  int32_t baseline_shift;

  /** \brief which link does this letter belong to
   *
   * To specify the link use the links array in the layoutProperties class. This
   * value is an index into that vector, the value 0 means no link, all the others
   * are reduced by one to represent the link
   */
  size_t link;

  /** \brief create an empty attribute, no font, no language, no flags, no inlay, no baseline shift
   */
  CodepointAttributes_c(void) : lang(""), flags(0), inlay(0), baseline_shift(0), link(0) { }

  /** \brief comparison operator
   */
  bool operator==(const CodepointAttributes_c & rhs) const
  {
    return c == rhs.c && font == rhs.font && lang == rhs.lang
      && flags == rhs.flags && shadows.size() && rhs.shadows.size()
      && std::equal(shadows.begin(), shadows.end(), rhs.shadows.begin())
      && inlay == rhs.inlay && baseline_shift == rhs.baseline_shift
      && link == rhs.link;
  }

  /** \brief this operator is required for the interval container within the
   * attributeIndex_c class, do not use it
   * \note the interval container wants to accumulate information but
   * as we can not do that here, we simply replace the old values
   */
  CodepointAttributes_c operator += (const CodepointAttributes_c & rhs)
  {
    // when someone tries to overwrite the attributes, we take over the new ones
    c = rhs.c;
    font = rhs.font;
    lang = rhs.lang;
    flags = rhs.flags;
    shadows = rhs.shadows;
    inlay = rhs.inlay;
    baseline_shift = rhs.baseline_shift;
    link = rhs.link;

    return *this;
  }
};


/** \brief collection of codepointAttributes for the layouter.
 *
 * This class behaves a bit like a vector of codepointAttributed in that you can
 * get an attribute for an index. The index is of type size_t
 */
class AttributeIndex_c
{
  private:
    boost::icl::interval_map<size_t, CodepointAttributes_c> attr;

  public:
    /** \brief create an empty index
     *  \attention you have to add at least one attribute
     */
    AttributeIndex_c(void) { }

    /** \brief create an index where all entries have the same attribute
     *  \param a the attribute that all will share
     */
    AttributeIndex_c(const CodepointAttributes_c & a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(0, SIZE_MAX), a);
    }

    /** \brief set attributes for a single indices
     *  \param i index that will have the attribute
     *  \param a the attribute
     */
    void set(size_t i, CodepointAttributes_c a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(i, i), a);
    }

    /** \brief set attributes for a range of indices
     *  \param start first index that should have the attribute
     *  \param end first index that will no longer have the attribute
     *  \param a the attribute
     */
    void set(size_t start, size_t end, CodepointAttributes_c a)
    {
      attr += std::make_pair(boost::icl::interval<size_t>::closed(start, end), a);
    }

    /** \brief get the attribute for given index
     *  \param i the index for which the attribute is requested
     *  \return the requested attribute
     */
    const CodepointAttributes_c & get(size_t i) const
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
class Shape_c
{
  public:
    /** \brief get the left edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the left edge usable for this section of the y-axis in 1/64th pixels
     */
    virtual int32_t getLeft(int32_t top, int32_t bottom) const = 0;

    /** \brief get the right edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the right edge usable for this section of the y-axis in 1/64th pixels
     */
    virtual int32_t getRight(int32_t top, int32_t bottom) const = 0;

    /** \brief get the left edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the left outer edge for this section of the y-axis in 1/64th pixels
     */
    virtual int32_t getLeft2(int32_t top, int32_t bottom) const = 0;

    /** \brief get the right edge
     *  \param top top limit of the area to get the edge for
     *  \param bottom bottom limit
     *  \return the right outer edge for this section of the y-axis in 1/64th pixels
     */
    virtual int32_t getRight2(int32_t top, int32_t bottom) const = 0;
};

/** \brief concrete implementation of the shape that will allow layouting
 *  inside a rectangular with a certain width
 */
class RectangleShape_c : public Shape_c
{
  private:
    int32_t w;

  public:
    /** \brief construct the shape such that the area goes from 0 to width on the x-axis
     *  \param width width of the rectangle in 1/64th pixels
     */
    RectangleShape_c(int32_t width) : w(width) { }

    virtual int32_t getLeft(int32_t /*top*/, int32_t /*bottom*/) const { return 0; }
    virtual int32_t getLeft2(int32_t /*top*/, int32_t /*bottom*/) const { return 0; }
    virtual int32_t getRight(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
    virtual int32_t getRight2(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
};

/** \brief this structure contains information for the layouter how to layout the text
 */
class LayoutProperties_c
{
  public:

    enum {
      ALG_LEFT,          ///< layout left adjusted
      ALG_RIGHT,         ///< layout right adjusted
      ALG_CENTER,        ///< layout centred
      ALG_JUSTIFY_LEFT,  ///< layout justified and the last line left adjusted
      ALG_JUSTIFY_RIGHT  ///< layout justified and the last line right adjusted
    } align = ALG_LEFT;  ///< alignment that the text is supposed to have

    int32_t indent = 0;  ///< indentation of the first line in pixel

    bool ltr = true;     ///< is the base direction of the text left to right?

    /** \brief The font that defines the underline parameters.
     *
     * When underlines need to be placed they will be placed according to this font. That way
     * you get the same font, regardless of size of text or font.
     *
     * When not set, the underline will use the parameters of the font of each glyph.
     */
    Font_c underlineFont;

    /** \brief link URLs for the links used in the CodepointAttribute_c */
    std::vector<std::string> links;

    /** \brief Choose between fast and optimizing linebreak algorithm
     *
     * The fast linebreak algorithm simply breaks a line as soon as the next part
     * doesn't fit onto the line. The optimizing algorithm tries to limit raggedness
     * of the lines and tries to avoid hyphenating words
     */
    bool optimizeLinebreaks = true;
};


/** paragraph layouting function
 *
 * \param txt32 the utf-32 encoded text to layout, no control sequences exist, use "\n" for newlines
 * \param attr the attributes (colours, ...) for all the characters in the text
 * \param shape the shape that the final result is supposed to have
 * \param prop some parameters that the line breaking algorithm needs to give the result the expected
 *             shape (centre or justified, ...)
 * \param ystart the vertical starting point (in 1/64th pixels) of your output, the baseline of the first line
 *               of text will be shifted down from this position by the ascender of the line
 * \return the resulting layout
 *
 * \note: bidi control characters supported, are RLE, RLE, PDF, those don't need to have a valid attribute entry
 * all other characters in txt32 need to have one, or the function will behave wrongly
 *
 * TODO: instead of crashing, rather throw an exception in that case.
 */
TextLayout_c layoutParagraph(const std::u32string & txt32, const AttributeIndex_c & attr,
                             const Shape_c & shape, const LayoutProperties_c & prop, int32_t ystart = 0);

}

#endif
