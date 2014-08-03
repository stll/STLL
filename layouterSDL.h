#ifndef __LAYOUTER_SDL__
#define __LAYOUTER_SDL__

/** \file
 *  \brief SDL output driver
 */

#include "layouter.h"

#include <SDL.h>

#include <vector>

/** \brief display a single layout
 *  \param l layout to draw
 *  \param sx x position on the target surface
 *  \param sy y position on the target surface
 *  \param s target surface
 */
void showLayoutSDL(const textLayout_c & l, int sx, int sy, SDL_Surface * s);

class layoutInfo_c
{
  public:

    layoutInfo_c(textLayout_c l, int x, int y) : layout(l), sx(x), sy(y) { }

    textLayout_c layout;
    int sx, sy;
};

void showLayoutsSelf(int w, int h, const std::vector<layoutInfo_c> & data);

#endif
