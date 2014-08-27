#ifndef __LAYOUTER_SDL__
#define __LAYOUTER_SDL__

/** \file
 *  \brief SDL output driver
 */

#ifdef HAVE_SDL

#include "layouter.h"

#include <SDL.h>

#include <vector>

namespace STLL {

/** \brief display a single layout
 *  \param l layout to draw
 *  \param sx x position on the target surface
 *  \param sy y position on the target surface
 *  \param s target surface
 */
void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s);

}

#endif

#endif
