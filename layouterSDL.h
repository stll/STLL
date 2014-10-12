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
#ifndef STLL_LAYOUTER_SDL
#define STLL_LAYOUTER_SDL

/** \file
 *  \brief SDL output driver
 */

#include "layouterFont.h"

#include <SDL.h>

namespace STLL {

class TextLayout_c;

/** \brief class used to encapsulate image drawing
 *
 * When the showLayoutSDL routine needs to draw an image it will call the draw function in this
 * class to do the job. This allows you to do your own image loading and caching and such stuff.
 *
 * Derive from this function and implement the draw function to handle image drawing in your application
 */
class imageDrawerSDL_c
{
  public:
    /** \brief function called to draw an image
     *
     * \param x x-position to draw the image in 1/64 pixels
     * \param y y-position to draw the image in 1/64 pixels
     * \param w width of the image to draw
     * \param h height of the image to draw
     * \param s the SDL-Surface to draw the image on
     * \param url the url of the image to draw
     */
    virtual void draw(int32_t x, int32_t y, uint32_t w, uint32_t h, SDL_Surface * s, const std::string & url) = 0;
};

/** \brief display a single layout
 *  \param l layout to draw
 *  \param sx x position on the target surface in 1/64th pixels
 *  \param sy y position on the target surface in 1/64th pixels
 *  \param s target surface
 *  \param sp which kind of sub-pixel positioning do you want?
 *  \param images a pointer to an image drawer class that is used to draw the images, when you give
 *                a nullptr here, no images will be drawn
 */
void showLayoutSDL(const TextLayout_c & l, int sx, int sy, SDL_Surface * s,
                   SubPixelArrangement sp, imageDrawerSDL_c * images);

/** \brief trims the font cache down to a maximal number of entries
 *
 * the SDL output module keeps a cache of rendered glyphs to speed up the process of
 * outputting layouts. This cache may get too big on memory. To keep things within limits
 * you can call this function to remove entries.
 * If there are more entries in the cache the ones that were used the longest time ago are removed
 *
 * \param num maximal number of entries, e.g. 0 completely empties the cache
 */
void trimSDLFontCache(size_t num);

}

#endif
