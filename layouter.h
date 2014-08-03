#ifndef __LAYOUTER_H__
#define __LAYOUTER_H__

/** \file
 *  \brief the paragraph layouter and its helpers
 */

#include "layouterFont.h"

#include <string>
#include <vector>
#include <memory>

/** encapsulates a finished layout.
 *
 * This class encapsulates a layout, it is a list of drawing commands
 */
class textLayout_c
{
  private:
    uint32_t height; // position where you can add additional text after this ...

  public:

    /** A drawing command
     */
    typedef struct
    {
      enum
      {
        CMD_GLYPH,  ///< draw a glyph from a font
        CMD_RECT    ///< draw a rectangle
      } command;    ///< specifies what to draw

      /// position of the glyph or upper left corner of rectangle
      int32_t x, y;

      /// which glyph to draw
      FT_UInt glyphIndex;

      /// which front to take the glyph from
      std::shared_ptr<fontFace_c> font;

      /// width and hight of the box to draw
      uint32_t w, h;

      // color of the glyph or the rectangle
      uint8_t r, g, b, a;

    } commandData;

    // order is the logical order, so when you do copy and paste we copy out of this using the order
    // within the vector
    // TODO find better solution instead of public vector
    std::vector<commandData> data;

    void addCommand(const commandData & d)
    {
      data.push_back(d);
    }

    void addCommandStart(const commandData & d)
    {
      data.insert(data.begin(), d);
    }

    void addCommandVector(const std::vector<commandData> & d, int dx, int dy)
    {
      for (auto a : d)
      {
        a.x += dx;
        a.y += dy;
        data.emplace_back(a);
      }
    }

    void append(const textLayout_c & l)
    {
      for (auto & a : l.data)
        data.push_back(a);

      height = std::max(height, l.height);
    }

    void operator=(textLayout_c && l)
    {
      data.swap(l.data);
      height = l.height;
    }

    void operator=(const textLayout_c & l)
    {
      data = l.data;
      height = l.height;
    }

    ~textLayout_c(void) { }

    textLayout_c(void) : height(0) { }

    textLayout_c(const textLayout_c & src) : height(src.height), data(src.data)  {  }

    textLayout_c(textLayout_c && src)
    {
      swap(data, src.data);
      height = src.height;
    }

    uint32_t getHeight(void) const { return height; }
    void setHeight(uint32_t h) { height = h; }
};

/** \brief this structure contains all attributes that a single glyph can get assigned
 */
typedef struct
{
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

  // TODO things like underline and strike-through are missing
  // TODO shadows
} codepointAttributes;


/** \brief collection of codepointAttributes for the layouter.
 *
 * This class behaves a bit like a vector of codepointAttributed in that you can
 * get an attribute for an index. The index is of type size_t
 */
class attributeIndex_c
{
  private:
    std::vector<codepointAttributes> attr;

  public:
    /** \brief create an empty index
     *  \attention you have to add at least one attribute
     */
    attributeIndex_c(void) { }

    /** \brief create an index where all entries have the same attribute
     *  \param a the attribute that all will share
     */
    attributeIndex_c(const codepointAttributes & a) { attr.push_back(a); }

    /** \brief set attributes for a singe of indices
     *  \param i index that will have the attribute
     *  \param a the attribute
     */
    void set(size_t i, codepointAttributes a)
    {
      if (i >= attr.size())
        attr.resize(i+1);
      attr[i] = a;
    }

    /** \brief set attributes for a range of indices
     *  \param start first index that should have the attribute
     *  \param end first index that will no longer have the attribute
     *  \param a the attribute
     */
    void set(size_t start, size_t end, codepointAttributes a)
    {
      if (end > attr.size())
        attr.resize(end);

      for (size_t i = start; i < end; i++)
        attr[i] = a;
    }

    /** \brief get the attribute for given index
     *  \param i the index for which the attribute is requested
     *  \return the requested attribute
     */
    const codepointAttributes & get(size_t i) const
    {
      if (i < attr.size())
        return attr[i];
      else
        return attr[0];
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
    virtual int32_t getRight(int32_t /*top*/, int32_t /*bottom*/) const { return w; }
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

} layoutProperties;


// base layout function that does the layouting stuff for one paragraph
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

#endif
